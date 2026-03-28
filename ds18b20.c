#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include "ds18b20.h"

#define W1_BASE_PATH 	"/sys/bus/w1/devices/"

static int find_sn(char *sn, size_t sn_len)
{
	DIR				*dirp = NULL;
	struct dirent 	*direntp = NULL;
	int 			found = 0;
	if(sn == NULL || sn_len == 0)
	{
		printf("sn/sn_len can't be NULL\/0\n");
		return -6;
	}

	dirp = opendir(W1_BASE_PATH);
	if(!dirp)
	{
		printf("open file % s failure: %s\n", W1_BASE_PATH, strerror(errno));
		return -1;
	}

	while((direntp = readdir(dirp)) != NULL)
	{
		if(strstr(direntp->d_name, "28-"))
		{
			strncpy(sn, direntp->d_name, sn_len);
			sn[sn_len - 1] = '\0';
			found = 1;
			break;
		}
	}

	closedir(dirp);

	if(!found)
	{
		printf("find "28-" file faliure: %s\n", strerror(errno));
		return -2;
	}

	return 0;

}


static int read_data(const char *sn, char *buf, size_t buf_len)
{
	char 		w1_path[128];
	int			file_fd = -1;
	ssize_t 	rt;

	memset(w1_path, 0, sizeof(w1_path));
	strncpy(w1_path, W1_BASE_PATH， sizeof(w1_path));
	strncat(w1_path, sn, sizeof(w1_path)-strlen(w1_path));
	strncat(w1_path, "/w1_slave", sizeof(w1_path)-strlen(w1_path));

	fd = open(w1_path, O_RDONLY);
	if(fd < 0)
	{
		printf("open file %s failure: %s\n", w1_path, strerror(errno));
		return -1;
	}

	memset(buf, 0, buf_len);
	rt = read(file_fd, buf, buf_len);
	if(rt <= 0)
	{
		printf("read data from fd[%d] failure: %s\n", file_fd, strerror(errno));
		close(file_fd);
		return -2;
	}

	close(file_fd);
	return 0;
}

int 	read_temperature(float *temperature)
{
	char		sn[64];
	char		buf[128];
	int			ret;
	char		*ptr = NULL;

	if(temperature == NULL)
	{
		printf("temperature pointer is NULL\n");
		return -6;
	}

	ret = find_sn(sn, sizeof(sn));
	if(ret < 0)
	{	
		return ret;
	}

	ret = read_data(sn, buf, sizeof(buf));
	if(ret < 0)
	{
		return ret-10;
	}

	ptr = strstr(buf, "t=");
	if(!ptr)
	{
		printf("find temperature failure: %s\n", strerror(errno));
		return -5;
	}

	ptr += 2;
	*temperature = atof(ptr)/1000.0;
	
	return 0;
}

