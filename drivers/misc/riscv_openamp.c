
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <asm/types.h>
#include <asm/sbi.h>

enum rproc_state {
	RPROC_STOPPED	= 0,
	RPROC_RUNNING	= 1,
	RPROC_LAST	= 2,
};

struct openamp_drv {
	/* physical address for remote proc start */
	u64			paddr;
	unsigned long		hartid;
	/* remote proc state */
	enum rproc_state	state;
};

struct openamp_drv oa;

struct kobject *openamp_kobj;

static int sbi_hsm_hart_start(unsigned long hartid, unsigned long saddr,
			      unsigned long priv)
{
	struct sbiret ret;

	ret = sbi_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_START,
			hartid, saddr, priv, 0, 0, 0);
	if (ret.error)
		return sbi_err_map_linux_errno(ret.error);
	else
		return 0;
}

static int sbi_hsm_hart_stop_remote(unsigned long hartid)
{
	struct sbiret ret;

	ret = sbi_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_STOP_REMOTE,
			hartid, 0, 0, 0, 0, 0);
	if (ret.error)
		return sbi_err_map_linux_errno(ret.error);
	else
		return 0;
}

static int sbi_hsm_hart_get_status(unsigned long hartid)
{
	struct sbiret ret;

	ret = sbi_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_STATUS,
			hartid, 0, 0, 0, 0, 0);
	if (ret.error)
		return sbi_err_map_linux_errno(ret.error);
	else
		return ret.value;
}

static enum rproc_state riscv_openamp_get_status(unsigned long hartid)
{
	int error;

	error = sbi_hsm_hart_get_status(oa.hartid);
	switch(error) {
		case SBI_HSM_HART_STATUS_STOP_PENDING:
		case SBI_HSM_HART_STATUS_STARTED:
			return RPROC_RUNNING;
			break;
		case SBI_HSM_HART_STATUS_START_PENDING:
		case SBI_HSM_HART_STATUS_STOPPED:
			return RPROC_STOPPED;
			break;
		default:
			return RPROC_LAST;
			break;
	}
}

static const char * const rproc_state_string[] = {
	[RPROC_STOPPED]		= "offline",
	[RPROC_RUNNING]		= "running",
	[RPROC_LAST]		= "invalid",
};

static ssize_t hartid_show(struct kobject *kobj,
			  struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", oa.hartid);
}

static ssize_t hartid_store(struct kobject *kobj,
			   struct kobj_attribute *attr,
			   const char *buf, size_t count)
{
	int ret;
	long long hartid;

	ret = kstrtoll(buf, 0, &hartid);
	if (ret)
		return ret;

	oa.hartid = hartid;
	if (oa.hartid < 0) {
		oa.hartid = -1;
		return -1;
	} else {
		return count;
	}
}

static ssize_t paddr_show(struct kobject *kobj,
			  struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%llx\n", oa.paddr);
}

static ssize_t paddr_store(struct kobject *kobj,
			   struct kobj_attribute *attr,
			   const char *buf, size_t count)
{
	int ret;
	long long paddr;

	ret = kstrtoll(buf, 0, &paddr);
	if (!ret) {
		oa.paddr = paddr;
		return count;
	} else {
		return ret;
	}
}

static ssize_t state_show(struct kobject *kobj,
			  struct kobj_attribute *attr, char *buf)
{
	unsigned int state;

	state = oa.state > RPROC_LAST ? RPROC_LAST : oa.state;
	return sprintf(buf, "%s\n", rproc_state_string[state]);
}

static ssize_t state_store(struct kobject *kobj,
			   struct kobj_attribute *attr,
			   const char *buf, size_t count)
{
	size_t ret = -1;

	oa.state = riscv_openamp_get_status(oa.hartid);
	if (sysfs_streq(buf, "start")) {
		if (oa.state == RPROC_RUNNING)
			return -EBUSY;

		ret = sbi_hsm_hart_start(oa.hartid, oa.paddr, 0);
		if (ret)
			pr_err("hart start failed\n");

		oa.state = riscv_openamp_get_status(oa.hartid);
	} else if (sysfs_streq(buf, "stop")) {
		ret = sbi_hsm_hart_stop_remote(oa.hartid);
		if (ret)
			pr_err("hart stop failed\n");

		ret = 0;
		oa.state = riscv_openamp_get_status(oa.hartid);
	} else {
		pr_err("invalid state\n");
	}
	if (ret == 0)
		return count;
	else
		return ret;
}

static struct kobj_attribute openamp_attr_state  = __ATTR_RW_MODE(state, 0600);
static struct kobj_attribute openamp_attr_hartid = __ATTR_RW_MODE(hartid, 0600);
static struct kobj_attribute openamp_attr_paddr  = __ATTR_RW_MODE(paddr, 0600);

static struct attribute *openamp_subsys_attrs[] = {
	&openamp_attr_state.attr,
	&openamp_attr_hartid.attr,
	&openamp_attr_paddr.attr,
	NULL,
};

umode_t __weak oa_attr_is_visible(struct kobject *kobj, struct attribute *attr,
				   int n)
{
	return attr->mode;
}

static const struct attribute_group openamp_subsys_attr_group = {
	.attrs = openamp_subsys_attrs,
	.is_visible = oa_attr_is_visible,
};

static int __init riscv_openamp_init(void)
{
	int error;

	memset(&oa, 0, sizeof(oa));
	oa.state = RPROC_LAST;
	openamp_kobj = kobject_create_and_add("openamp", firmware_kobj);
	if (!openamp_kobj) {
		pr_err("openamp: Firmware registration failed.\n");
		return -ENOMEM;
	}

	error = sysfs_create_group(openamp_kobj, &openamp_subsys_attr_group);
	if (error) {
		pr_err("efi: Sysfs attribute export failed with error %d.\n",
		       error);
		goto err_put;
	}

	oa.state = riscv_openamp_get_status(oa.hartid);
	return error;
err_put:
	kobject_put(openamp_kobj);
	return error;
}
device_initcall(riscv_openamp_init);
