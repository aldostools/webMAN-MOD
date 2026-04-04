// 3141card
#ifndef __VSHTASK_H__
#define __VSHTASK_H__


extern int vshtask_0F80B71F(const char *path);  // auth_module, checks sprx SCE header 

extern int vshtask_166551C5(char *app_id);      // RTC Alarm Unregister

//extern int vshtask_668E3C94();  // some RTC Alarm Register 	int vshtask_668E3C94(char *app_id?, uint8_t [0x4C]) 

// vshtask_8D03E0FD               // ?

extern int vshtask_A02D46E7(int arg, const char *msg);  // vshtask_notification_msg()
#define vshtask_notify(msg) vshtask_A02D46E7(0, msg)

#endif // __VSHTASK_H__ 
