//Use Gumdo Parser to extract href -> https://github.com/google/gumbo-parser
//Use libtask to coroutine

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <task.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include "extract_urls.h"

enum
{
	STACK = 32768
};

char server[PATH_MAX];
char url[] = "/";
char dir_name[PATH_MAX];
std::set <std::string> downloaded_files;
std::set <std::pair <int, std::string>> parametrs;

struct tmp {
	std::set <std::pair <int, std::string>>::iterator iter;
};

void fetchtask(void*);

//create directory with all path
static void mkdir_p(const char *dir) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, PATH_MAX, "%s", dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    for (p = tmp + 1; *p; p++) {
        if(*p == '/') {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    }
    mkdir(tmp, 0777);
    std::cout << strerror(errno) << std::endl;
}

//create directories for url
size_t create_dirs (char * url)
{
	if (url[strlen(url) - 1] == '/') {
		url[strlen(url) - 1] == 0;
	}

	size_t end_path = std::string(url).find_last_of('/');
	if (end_path > strlen(url) - 1) {
    } else {
    	std::string dir_path = std::string(url).substr(0, end_path);
	    std::cout << "CREATE OR CHECK DIRECTORY:\t" << dir_path << std::endl;
	    mkdir_p(dir_path.c_str());
    }

    return end_path;
}


void
taskmain(int argc, char **argv)
{
	int i, n;

	strcpy(server, argv[1]);

	char name_main_dict[PATH_MAX];
	snprintf(name_main_dict, PATH_MAX, "./%s", argv[1]);
	int t = mkdir(name_main_dict, 0777);

	strcpy(dir_name, name_main_dict);

	auto param = std::make_pair(atoi(argv[2]), "/");
	auto p = parametrs.insert(param);

	struct tmp tm;
	tm.iter = p.first;

	taskcreate(fetchtask, (void *)&tm, STACK);

	while(taskyield() > 1);
	sleep(1);

	taskexit(0);
}

void
fetchtask(void *param)
{
	setvbuf(stdout, NULL, _IOLBF, 0);
	setvbuf(stdin, NULL, _IOLBF, 0);
	
	int fd, n;
	char buf[4096];
	FILE *f;
	std::set<std::string> urls;
	int i;

	auto iter_param = ((struct tmp *) param)->iter;
	int deep = iter_param->first;

	char url_path[PATH_MAX];
	strcpy(url_path, iter_param->second.c_str());

	char os_path[PATH_MAX];
	if (url_path[0] == '/') {
		snprintf(os_path, PATH_MAX, "%s%s.html", dir_name, url_path);	
	} else {
		snprintf(os_path, PATH_MAX, "%s/%s.html", dir_name, url_path);
	}

	if (strcmp(url_path, "/") == 0) {
		snprintf(os_path, PATH_MAX, "%s/%s.html", dir_name, server);
		std::cout << "FIRST URL" <<std::endl;
	}

	// sleep(0.1);
	if ((fd = netdial(TCP, server, 80)) < 0) {
		printf("Error!\n");
		fprintf(stderr, "dial %s: %s (%s)\n", url_path, strerror(errno), taskgetstate());
		close(fd);
		parametrs.erase(iter_param);
		taskexit(0);
	}

	std::cout << "_______________________________________________________________" << std::endl;

	snprintf(buf, sizeof buf, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", url_path, server);
	fdwrite(fd, buf, strlen(buf));
	printf("%s", buf);

	size_t name_begin = create_dirs(os_path); 

	std::cout << "CREATING_FILE:  " << os_path << "\tDEEP:  " << deep <<std::endl << std::endl;

	if (!(f = fopen(os_path, "w"))) {
		std::cout << strerror(errno) << std::endl;
		close(fd);
		parametrs.erase(iter_param);
		taskexit(0);
	}
	
	// std::cout << strerror(errno) << std::endl;
	int check = 0;
	int idx = 0;
	int del_server_answer = 0;
	char *begin;
	while((n = fdread(fd, buf, sizeof(buf))) > 0){
		if (!del_server_answer) {
			while (1) {
				if (!strncmp(buf + idx, "\r\n\r\n", 4)) {
					begin = buf + idx + 4;
					n = n-4;
					del_server_answer = 1;
					break;
				}
				++idx;
				--n;
			}
		} else {
			begin = buf;
		}
		fwrite (begin, n, sizeof(char), f);
		check = 1;
	}

	if (check == 0) {
		std::cout << "DELETE FILE" << std::endl;
		remove(os_path);
		close(fd);
		parametrs.erase(iter_param);
		taskexit(0);
	} else {
		fclose(f);
		urls.clear();
		extract_urls(os_path, urls);
	}

	close(fd);

	if (deep != 0) {

		for (auto iter = urls.begin(); iter != urls.end(); ++iter){
			// std::cout <<"NEW_URL:  " << *iter << std::endl;

			if ((*iter)[0] == '/') {
				snprintf(url_path, PATH_MAX, "%s", (*iter).c_str());
			} else {
				snprintf(url_path, PATH_MAX, "/%s", (*iter).c_str());
			}

			if (downloaded_files.find(std::string(url_path)) != downloaded_files.end()) {
				std::cout << url_path << "\tALREADY DOWNLOAD" << std::endl;
				continue;
			}

			downloaded_files.insert(std::string(url_path));
			
			auto param_ = std::make_pair(deep - 1, std::string(url_path));
			auto p_ = parametrs.insert(param_);

			struct tmp* tm = (struct tmp *)calloc(1, sizeof(struct tmp));
			tm -> iter = p_.first;

			taskcreate(fetchtask, (void *)tm, STACK);
		}
	}

	urls.clear();
	parametrs.erase(iter_param);
	// std::cout << "FILES\t" << downloaded_files.size() << " parametrs\t" << parametrs.size() << std::endl;
	taskexit(0);
}
