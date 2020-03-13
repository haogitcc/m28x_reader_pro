#ifndef _MODULE_INIT_H
#define _MODULE_INIT_H

#ifdef  __cplusplus
extern "C" {
#endif

int module_ant_flags(int port);
int module_ant_judge(int ant1, int ant2, int ant3, int ant4);
int module_ant_check();
int module_get_ant();
void module_ant_power();
int module_init();
int module_clear();
int module_gpitriger();
void module_startReading();
void mid_tagdata_send(int flags);
void mid_tagdata_send_v(int flag);
void mid_tagdata_nocache(char *epcstr, char *time, int ant);
void mid_tagdata_cache(int flags);
void socket_data_close();
int get_ant_status();


#ifdef __cplusplus
}
#endif

#endif

