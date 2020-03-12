
/*******************************************************************************
	公司：
			Yuxing software
	纪录：
			2008-12-8 11:26:52 create by Liu Jianhua
	模块：
			netwk
	简述：
			telnetd
 *******************************************************************************/

#ifndef __TELNETD_H__
#define __TELNETD_H__

#ifdef __cplusplus
extern "C" {
#endif

#define TD_ARGC_MAX		5

/*
	argc：
		参数个数，也就是 参数数组 的元素个数
	argv：
		参数数组，argv[0] 为第1个参数，argv[1]为第二个参数
 */
typedef int (*td_callfunc)(int argc, char **argv);

//void mid_netwk_buildtime(void);

//#define telnetd_init telnetd_init_v1
int telnetd_init(int active);

void telnetd_active(void);
void telnetd_suspend(void);

/*
	callname：
		命令名称，长度要求小于16 尽可长度短且一律为小写字符，以利于telnet端输入。
	func：
		命令对应的函数
	argstype:
		 由'd'，'s'和'e'组成的参数类型字符串， 'd' 数字，'s' 字符串，'e' 结尾字符串，和's'意义相同，但'e'只能为最后一个参数，可以包含一些特殊字符。
 */
int telnetd_regist(const char *callname, td_callfunc func, const char *argstype);

int telnetd_output(const char *buf, unsigned int len);


/*
举例1：iptv，频道播放命令，该命令只有一个字符串参数，就是频道播放URL

static int shell_iptv(int argc, char **argv)
{
	mid_stream_open(0, argv[0], APP_TYPE_IPTV, -1);
	return 0;
}
......
telnetd_regist("iptv",			shell_iptv,				"s");
......

举例2：rect，设置视频区域命令，该命令只有四个数字参数：横坐标、纵坐标、宽和高。
static int shell_rect(int argc, char **argv)
{
	int x, y, w, h;

	x = atoi(argv[0]);
	y = atoi(argv[1]);
	w = atoi(argv[2]);
	h = atoi(argv[3]);

	mid_stream_rect(0, x, y, w, h);

	return 0;
}
......
telnetd_regist("rect",			shell_rect,				"dddd");
......
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELNETD_H__ */
