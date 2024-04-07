
#define LIBRARY_IMPL (1)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <orbis/libkernel.h>
#include <orbis/Sysmodule.h>
#include <sys/socket.h>
#include "appcont.h"
#include <string.h>

// from open orbis crtlib.c
extern char __text_start;
void (*__init_array_start[])(void);
void (*__init_array_end[])(void);

// sce_module_param
__asm__(
	".intel_syntax noprefix \n"
	".align 0x8 \n"
	".section \".data.sce_module_param\" \n"
	"_sceProcessParam: \n"
	// size
	"	.quad 	0x18 \n"
	// magic
	"	.quad   0x13C13F4BF \n"
	// SDK version
	"	.quad 	0x1000051 \n"
	".att_syntax prefix \n");

// data globals
__asm__(
	".intel_syntax noprefix \n"
	".align 0x8 \n"
	".data \n"
	"__dso_handle: \n"
	"	.quad 	0 \n"
	"_sceLibc: \n"
	"	.quad 	0 \n"
	".att_syntax prefix \n");

// this does not get called ever?
int module_start(int64_t args, const void *argp)
{
	__asm__(
		".intel_syntax noprefix \n"
		"ud2 \n");
	for (void (**i)(void) = __init_array_start; i != __init_array_end; i++)
	{
		i[0]();
	}

	return 0;
}

int module_stop(int64_t args, const void *argp)
{
	return 0;
}

#define SCE_SYSMODULE_APP_CONTENT 0x00b4

int32_t DEBUG_MODE = -1; // injected by main script

// snprintf crashes if used in init...?
int32_t _init()
{
	if (DEBUG_MODE != 0)
	{
		// delete previous log
		sceKernelUnlink("/data/dlcldr.log");
		Log("[dlcldr] init");
	}

	int res = sceSysmoduleLoadModule(SCE_SYSMODULE_APP_CONTENT);

	if (res != SCE_OK)
	{
		// we'll crash so might as well log
		char log_buf[250];
		strcat(log_buf, "[dlcldr] sceSysmoduleLoadModule call failed for SCE_SYSMODULE_APP_CONTENT. res: ");
		char res_str[10];
		intToStr(res, res_str);
		strcat(log_buf, res_str);
		Log(log_buf);
		return -1;
	}

	if (DEBUG_MODE != 0)
	{
		Log("[dlcldr] loaded libSceAppContent");
	}

	SceAppContentInitParam initParam;
	SceAppContentBootParam bootParam;
	memset(&initParam, 0, sizeof(SceAppContentInitParam));
	memset(&bootParam, 0, sizeof(SceAppContentBootParam));
	res = sceAppContentInitialize(&initParam, &bootParam);

	if (res < 0)
	{
		// we'll crash so might as well log
		char log_buf[250];
		strcat(log_buf, "[dlcldr] sceAppContentInitialize call failed. res: ");
		char res_str[10];
		intToStr(res, res_str);
		strcat(log_buf, res_str);
		Log(log_buf);
		return -1;
	}

	if (DEBUG_MODE != 0)
	{
		Log("[dlcldr] initialized libSceAppContent");
	}

	return 0;
}

int32_t _fini()
{
	return 0;
}

void intToStr(int num, char *str)
{
	int i = 0;
	int isNegative = 0;

	// If the number is negative, make it positive and set the flag
	if (num < 0)
	{
		isNegative = 1;
		num = -num;
	}

	// Handle 0 explicitly, otherwise empty string is printed for 0
	if (num == 0)
	{
		str[i++] = '0';
		str[i] = '\0';
		return;
	}

	// Process individual digits
	while (num != 0)
	{
		int digit = num % 10;
		str[i++] = digit + 0x30; // Convert digit to its ASCII character
		num = num / 10;
	}

	// If the number was negative, append '-'
	if (isNegative)
		str[i++] = '-';

	str[i] = '\0'; // Append string terminator

	// Reverse the string
	int start = 0;
	int end = i - 1;
	while (start < end)
	{
		char temp = str[start];
		str[start] = str[end];
		str[end] = temp;
		start++;
		end--;
	}
}

void Log(const char *str)
{
	char *new_str = (char *)malloc(strlen(str) + 2);
	strcpy(new_str, str);
	strcat(new_str, "\n");

	sceKernelDebugOutText(0, new_str);
	append_to_log_file(new_str);
	free(new_str);

	if (DEBUG_MODE == 2)
	{
		OrbisNotificationRequest Buffer;
		memset(&Buffer, 0, sizeof(OrbisNotificationRequest));
		strncpy(Buffer.message, str, 1024);
		Buffer.targetId = -1;

		int res = sceKernelSendNotificationRequest(0, &Buffer, sizeof(Buffer), 0);
	}
}

int append_to_log_file(const char *str)
{
	int fd = sceKernelOpen("/data/dlcldr.log", SCE_KERNEL_O_WRONLY | SCE_KERNEL_O_CREAT | SCE_KERNEL_O_APPEND, SCE_KERNEL_S_IRWU);

	if (fd <= 0)
	{
		return -1;
	}

	size_t len = strlen(str);
	sceKernelWrite(fd, str, len);

	sceKernelClose(fd);

	return 0;
}

int32_t dlcldr_sceAppContentInitialize(
	SceAppContentInitParam *initParam,
	SceAppContentBootParam *bootParam)
{
	if (DEBUG_MODE != 0)
	{
		Log("[dlcldr] dlcldr_sceAppContentInitialize called");
	}
	return 0;
}

int32_t dlcldr_sceAppContentAppParamGetInt(
	SceAppContentAppParamId paramId,
	int32_t *value)
{
	// patch trial flag
	if (paramId == SCE_APP_CONTENT_APPPARAM_ID_SKU_FLAG)
	{
		if (DEBUG_MODE != 0)
		{
			Log("[dlcldr] dlcldr_sceAppContentAppParamGetInt called for SKU_FLAG, returning FULL");
		}
		*value = SCE_APP_CONTENT_APPPARAM_SKU_FLAG_FULL;
		return 0;
	}

	if (DEBUG_MODE != 0)
	{
		Log("[dlcldr] dlcldr_sceAppContentAppParamGetInt proxy called");
	}

	return sceAppContentAppParamGetInt(paramId, value);
}

int32_t addcont_count = -1;

dlcldr_struct addcontInfo[SCE_APP_CONTENT_INFO_LIST_MAX_SIZE] = {
	{{"0000000000000000"}, 4, {0xCA, 0xB6, 0xD7, 0xB1, 0xEC, 0x15, 0x39, 0xCE, 0xCE, 0xEE, 0xE9, 0x00, 0x53, 0xE6, 0x91, 0xC9}},
};

int32_t dlcldr_sceAppContentGetAddcontInfoList(
	SceNpServiceLabel serviceLabel,
	SceAppContentAddcontInfo *list,
	uint32_t listNum,
	uint32_t *hitNum)
{
	if (DEBUG_MODE != 0)
	{
		char log_buf[250];
		snprintf(log_buf, 250, "[dlcldr] dlcldr_sceAppContentGetAddcontInfoList called with listNum: %d", listNum);
		Log(log_buf);
	}

	if (listNum == 0)
	{
		*hitNum = addcont_count;
		return 0;
	}

	if (list == NULL)
	{
		return 0;
	}

	int dlcsToList = addcont_count < listNum ? addcont_count : listNum;

	for (int i = 0; i < dlcsToList; i++)
	{
		strncpy(list[i].entitlementLabel.data, addcontInfo[i].entitlementLabel, SCE_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE);
		list[i].status = addcontInfo[i].status;
	}

	if (hitNum != NULL)
	{
		*hitNum = dlcsToList;
	}

	if (DEBUG_MODE != 0)
	{
		char log_buf[250];
		snprintf(log_buf, 250, "[dlcldr] dlcldr_sceAppContentGetAddcontInfoList returned %d dlc(s)", dlcsToList);
		Log(log_buf);
	}

	return 0;
}

int32_t dlcldr_sceAppContentGetAddcontInfo(
	SceNpServiceLabel serviceLabel,
	const SceNpUnifiedEntitlementLabel *entitlementLabel,
	SceAppContentAddcontInfo *info)
{
	if (DEBUG_MODE != 0)
	{
		char log_buf[250];
		snprintf(log_buf, 250, "[dlcldr] dlcldr_sceAppContentGetAddcontInfo called for %s", entitlementLabel->data);
		Log(log_buf);
	}
	for (int i = 0; i < addcont_count; i++)
	{
		if (memcmp(entitlementLabel->data, addcontInfo[i].entitlementLabel, 16) == 0)
		{
			if (DEBUG_MODE != 0)
			{
				char log_buf[250];
				snprintf(log_buf, 250, "[dlcldr] dlcldr_sceAppContentGetAddcontInfo found %s", entitlementLabel->data);
				Log(log_buf);
			}

			strncpy(info->entitlementLabel.data, addcontInfo[i].entitlementLabel, SCE_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE);
			info->status = addcontInfo[i].status;
			return 0;
		}
	}

	if (DEBUG_MODE != 0)
	{
		char log_buf[250];
		snprintf(log_buf, 250, "[dlcldr] dlcldr_sceAppContentGetAddcontInfo did not find %s, returning SCE_APP_CONTENT_ERROR_DRM_NO_ENTITLEMENT", entitlementLabel->data);
		Log(log_buf);
	}

	return SCE_APP_CONTENT_ERROR_DRM_NO_ENTITLEMENT;
}

int32_t dlcldr_sceNpEntitlementAccessGetAddcontEntitlementInfoList(
	SceNpServiceLabel serviceLabel,
	SceNpEntitlementAccessAddcontEntitlementInfo *list,
	uint32_t listNum,
	uint32_t *hitNum)
{
	if (listNum == 0)
	{
		*hitNum = addcont_count;
		return 0;
	}

	if (list == NULL)
	{
		return 0;
	}

	int list_count = listNum < addcont_count ? listNum : addcont_count;

	for (int i = 0; i < list_count; i++)
	{
		strncpy(list[i].entitlementLabel.data, addcontInfo[i].entitlementLabel, SCE_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE);
		list[i].downloadStatus = addcontInfo[i].status;
		list[i].packageType = SCE_NP_ENTITLEMENT_ACCESS_PACKAGE_TYPE_PSAC;
	}

	if (hitNum != NULL)
	{
		*hitNum = list_count;
	}

	return 0;
}

int32_t dlcldr_sceNpEntitlementAccessGetAddcontEntitlementInfo(
	SceNpServiceLabel serviceLabel,
	const SceNpUnifiedEntitlementLabel *entitlementLabel,
	SceNpEntitlementAccessAddcontEntitlementInfo *info)
{
	for (int i = 0; i < addcont_count; i++)
	{
		if (memcmp(entitlementLabel->data, addcontInfo[i].entitlementLabel, 16) == 0)
		{
			strncpy(info->entitlementLabel.data, addcontInfo[i].entitlementLabel, SCE_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE);
			info->downloadStatus = addcontInfo[i].status;
			info->packageType = SCE_NP_ENTITLEMENT_ACCESS_PACKAGE_TYPE_PSAC;
			return 0;
		}
	}

	return SCE_APP_CONTENT_ERROR_DRM_NO_ENTITLEMENT;
}

int32_t dlcldr_sceAppContentGetEntitlementKey(
	SceNpServiceLabel serviceLabel,
	const SceNpUnifiedEntitlementLabel *entitlementLabel,
	SceAppContentEntitlementKey *key)
{
	if (DEBUG_MODE != 0)
	{
		char log_buf[250];
		snprintf(log_buf, 250, "[dlcldr] dlcldr_sceAppContentGetEntitlementKey called for %s", entitlementLabel->data);
		Log(log_buf);
	}

	for (int i = 0; i < addcont_count; i++)
	{
		if (memcmp(entitlementLabel->data, addcontInfo[i].entitlementLabel, 16) == 0)
		{
			if (DEBUG_MODE != 0)
			{
				char log_buf[250];
				snprintf(log_buf, 250, "[dlcldr] dlcldr_sceAppContentGetEntitlementKey found key for %s", entitlementLabel->data);
				Log(log_buf);
			}
			memcpy(key->data, addcontInfo[i].key, SCE_APP_CONTENT_ENTITLEMENT_KEY_SIZE);
			return 0;
		}
	}

	if (DEBUG_MODE != 0)
	{
		char log_buf[250];
		snprintf(log_buf, 250, "[dlcldr] dlcldr_sceAppContentGetEntitlementKey did not find key for %s, returning SCE_APP_CONTENT_ERROR_DRM_NO_ENTITLEMENT", entitlementLabel->data);
		Log(log_buf);
	}

	return SCE_APP_CONTENT_ERROR_DRM_NO_ENTITLEMENT;
}

int32_t dlcldr_sceNpEntitlementAccessGetEntitlementKey(
	SceNpServiceLabel serviceLabel,
	const SceNpUnifiedEntitlementLabel *entitlementLabel,
	SceNpEntitlementAccessEntitlementKey *key)
{
	return dlcldr_sceAppContentGetEntitlementKey(serviceLabel, entitlementLabel, key);
}

int32_t dlcldr_sceAppContentAddcontDelete(
	SceNpServiceLabel serviceLabel,
	const SceNpUnifiedEntitlementLabel *entitlementLabel)
{
	return 0;
}

int32_t dlcldr_sceAppContentAddcontMount(
	SceNpServiceLabel serviceLabel,
	const SceNpUnifiedEntitlementLabel *entitlementLabel,
	SceAppContentMountPoint *mountPoint)
{
	if (DEBUG_MODE != 0)
	{
		char log_buf[250];
		snprintf(log_buf, 250, "[dlcldr] dlcldr_sceAppContentAddcontMount called for %s", entitlementLabel->data);
		Log(log_buf);
	}

	for (int i = 0; i < addcont_count; i++)
	{
		if (memcmp(entitlementLabel->data, addcontInfo[i].entitlementLabel, 16) == 0)
		{
			if (addcontInfo[i].status != SCE_APP_CONTENT_ADDCONT_DOWNLOAD_STATUS_INSTALLED)
			{
				if (DEBUG_MODE != 0)
				{
					char log_buf[250];
					snprintf(log_buf, 250, "[dlcldr] dlcldr_sceAppContentAddcontMount failed for %s, not installed", entitlementLabel->data);
					Log(log_buf);
				}
				return SCE_APP_CONTENT_ERROR_NOT_MOUNTED;
			}

			char new_mount_point[SCE_APP_CONTENT_MOUNTPOINT_DATA_MAXSIZE];

			if (i < 10)
			{
				// to avoid changing the naming convention
				snprintf(new_mount_point, SCE_APP_CONTENT_MOUNTPOINT_DATA_MAXSIZE, "/app0/dlc%02d", i);
			}
			else
			{
				snprintf(new_mount_point, SCE_APP_CONTENT_MOUNTPOINT_DATA_MAXSIZE, "/app0/dlc%d", i);
			}

			strncpy(mountPoint->data, new_mount_point, SCE_APP_CONTENT_MOUNTPOINT_DATA_MAXSIZE);

			if (DEBUG_MODE != 0)
			{
				char log_buf[250];
				snprintf(log_buf, 250, "[dlcldr] dlcldr_sceAppContentAddcontMount mounted %s at %s", entitlementLabel->data, new_mount_point);
				Log(log_buf);
			}
			return 0;
		}
	}

	if (DEBUG_MODE != 0)
	{
		char log_buf[250];
		snprintf(log_buf, 250, "[dlcldr] dlcldr_sceAppContentAddcontMount failed for %s, not found", entitlementLabel->data);
		Log(log_buf);
	}

	return SCE_APP_CONTENT_ERROR_DRM_NO_ENTITLEMENT;
}

int32_t dlcldr_sceAppContentAddcontUnmount(
	const SceAppContentMountPoint *mountPoint)
{
	return 0;
}

int32_t dlcldr_sceAppContentGetPftFlag(
	SceAppContentPftFlag *pftFlag)
{
	*pftFlag = SCE_APP_CONTENT_PFT_FLAG_OFF;
	return 0;
}

// typedef enum SceNpState
// {
// 	SCE_NP_STATE_UNKNOWN = 0,
// 	SCE_NP_STATE_SIGNED_OUT,
// 	SCE_NP_STATE_SIGNED_IN
// } SceNpState;
// int sceNpGetState(
// 		SceUserServiceUserId userId,
// 		SceNpState *state);
// int (*sceNpGetState)(SceUserServiceUserId userId, SceNpState *state) = NULL;

// gYOLMIKifpk
// int32_t dlcldr_sceNpGetState(
// 	SceUserServiceUserId userId,
// 	SceNpState *state)
// {
// 	*state = SCE_NP_STATE_SIGNED_IN;
// 	if (DEBUG_MODE != 0)
// 	{
// 		Log("[dlcldr] sceNpGetState faked signed_in");
// 	}
// 	return 0;
// }

// int32_t dlcldr_sceAppContentAppParamGetInt(
// 	SceAppContentAppParamId paramId,
// 	int32_t *value)
// {
// 	return 0;
// }

// int32_t dlcldr_sceAppContentAddcontEnqueueDownload(
// 	SceNpServiceLabel serviceLabel,
// 	const SceNpUnifiedEntitlementLabel *entitlementLabel)
// {
// 	return 0;
// }

// int32_t dlcldr_sceAppContentTemporaryDataMount2(
// 	SceAppContentTemporaryDataOption option,
// 	SceAppContentMountPoint *mountPoint)
// {
// 	return 0;
// }

// int32_t dlcldr_sceAppContentTemporaryDataUnmount(
// 	const SceAppContentMountPoint *mountPoint)
// {
// 	return 0;
// }

// int32_t dlcldr_sceAppContentTemporaryDataFormat(
// 	const SceAppContentMountPoint *mountPoint)
// {
// 	return 0;
// }

// int32_t dlcldr_sceAppContentTemporaryDataGetAvailableSpaceKb(
// 	const SceAppContentMountPoint *mountPoint,
// 	size_t *availableSpaceKb)
// {
// 	return 0;
// }

// int32_t dlcldr_sceAppContentDownloadDataFormat(
// 	const SceAppContentMountPoint *mountPoint)
// {
// 	return 0;
// }

// int32_t dlcldr_sceAppContentDownloadDataGetAvailableSpaceKb(
// 	const SceAppContentMountPoint *mountPoint,
// 	size_t *availableSpaceKb)
// {
// 	return 0;
// }

// int32_t dlcldr_sceAppContentGetAddcontDownloadProgress(
// 	SceNpServiceLabel serviceLabel,
// 	const SceNpUnifiedEntitlementLabel *entitlementLabel,
// 	SceAppContentAddcontDownloadProgress *progress)
// {
// 	return 0;
// }

// int32_t dlcldr_sceAppContentAddcontEnqueueDownloadByEntitlemetId() { return 0; }
// int32_t dlcldr_sceAppContentAddcontEnqueueDownloadSp() { return 0; }
// int32_t dlcldr_sceAppContentAddcontMountByEntitlemetId() { return 0; }
// int32_t dlcldr_sceAppContentAddcontShrink() { return 0; }
// int32_t dlcldr_sceAppContentAppParamGetString() { return 0; }
// int32_t dlcldr_sceAppContentDownload0Expand() { return 0; }
// int32_t dlcldr_sceAppContentDownload0Shrink() { return 0; }
// int32_t dlcldr_sceAppContentDownload1Expand() { return 0; }
// int32_t dlcldr_sceAppContentDownload1Shrink() { return 0; }
// int32_t dlcldr_sceAppContentGetAddcontInfoByEntitlementId() { return 0; }
// int32_t dlcldr_sceAppContentGetAddcontInfoListByIroTag() { return 0; }
// int32_t dlcldr_sceAppContentGetDownloadedStoreCountry() { return 0; }
// int32_t dlcldr_sceAppContentGetEntitlementKey() { return 0; }
// int32_t dlcldr_sceAppContentGetPftFlag() { return 0; }
// int32_t dlcldr_sceAppContentGetRegion() { return 0; }
// int32_t dlcldr_sceAppContentRequestPatchInstall() { return 0; }
// int32_t dlcldr_sceAppContentSmallSharedDataFormat() { return 0; }
// int32_t dlcldr_sceAppContentSmallSharedDataGetAvailableSpaceKb() { return 0; }
// int32_t dlcldr_sceAppContentSmallSharedDataMount() { return 0; }
// int32_t dlcldr_sceAppContentSmallSharedDataUnmount() { return 0; }
