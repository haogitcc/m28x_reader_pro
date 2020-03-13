#ifndef _TMR_COMMAND_H_
#define _TMR_COMMAND_H_

#ifdef  __cplusplus
extern "C" {
#endif

int findChildCnt(char* str1, char* str2);
void parse_server_port(char *str);
int parse_antenna(char *str);
int parse_command(char* vlaue);
void get_reader_status(char *msg_id, char* cmd, char *source);
void reponse_server(char *msg_id, char* cmd, int flags, char *source);
void get_server_reader();
int mid_tagdata_server(char* str, int number, int fd);
void get_server_time();


#ifdef __cplusplus
}
#endif

#endif

