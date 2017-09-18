#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined _WIN32 || defined __CYGWIN__
#include <windows.h>
#define USING_WINDOWS
#endif

#define PL_FORMAT_LEFT 		"%%F{%u}%%K{%u}%s"
#define PL_FORMAT_LEFT_END 	"%%F{%u}%%k%s%%f "

#define PL_FORMAT_RIGHT		"%%F{%u}%%K{%u}%s"
#define PL_FORMAT_RIGHT_END	"%%F{%u}%%k%s%%f"

#define D(a) std::cout << a << std::endl

struct pl_segment {
	uint8_t fg;
	uint8_t bg;
	std::string text;
	pl_segment(uint8_t _fg, uint8_t _bg, std::string _text) {
		fg = _fg;
		bg = _bg;
		text = _text;
	};
	pl_segment(uint8_t _fg, uint8_t _bg) {
		fg = _fg;
		bg = _bg;
	}
};

class pl_manager {
public:
	std::vector<pl_segment> segments_left;
	std::vector<pl_segment> segments_right;
	const char * lchar;
	const char * rchar;
	void render_segments_left() {
		if (segments_left.size() > 0) {
			uint32_t i=0;
			for (;i<(segments_left.size()-1);i++) {
				printf(
					"%%F{%u}%%K{%u}%s%%F{%u}%%K{%u}%s",
					segments_left[i].fg,
					segments_left[i].bg,
					segments_left[i].text.c_str(),
					segments_left[i].bg,
					segments_left[i+1].bg,
					lchar
				);
			}
			printf(
				"%%F{%u}%%K{%u}%s%%F{%u}%%k%s%%f ",
				segments_left[i].fg,
				segments_left[i].bg,
				segments_left[i].text.c_str(),
				segments_left[i].bg,
				lchar
			);
		}
	};
	void render_segments_right() {
		if (segments_right.size() > 0) {
			uint32_t i=0;
			printf(
				"%%F{%u}%s%%F{%u}%%K{%u}%s",
				segments_right[i].bg,
				rchar,
				segments_right[i].fg,
				segments_right[i].bg,
				segments_right[i].text.c_str()
			);
			i++;
			for (;i<segments_right.size();i++) {
				printf(
					"%%F{%u}%%K{%u}%s%%F{%u}%%K{%u}%s",
					segments_right[i].bg,
					segments_right[i-1].bg,
					rchar,
					segments_right[i].fg,
					segments_right[i].bg,
					segments_right[i].text.c_str()
				);
			}
		}
	};
};

inline void pad_space(std::string * target) {
	target->insert(0," ");
	target->append(" ");
}

int info_get_cwd_short(std::string * ostr) {
	char buf[512];
	if (getcwd(&buf[0],sizeof(buf)) != NULL) {
		size_t found;
		std::string wdstr(buf);
		std::string wdsstr;
		found = wdstr.find_last_of("/");
		if (found != std::string::npos) {
			wdsstr = wdstr.substr(found+1);
			//found2 = wdsstr.find_last_of("/");
			if (wdsstr != "") {
				*ostr = wdsstr;
			} else {
				*ostr = std::string("/");
			}
			return 1;
		} else {
			*ostr = std::string("/");
			return 1;
		}
		return 1;
	} else {
		*ostr = std::string("/");
		return 1;
	}
	return 0;
}

int info_get_cwd_full(std::string * ostr) {
	char buf[512];
	if (getcwd(&buf[0],sizeof(buf)) != NULL) {
		*ostr = std::string(buf);
		return 1;
	} else {
		return 0;
	}
	return 0;
}

int info_get_cwd_home(std::string * ostr) {
	std::string fullpath;
	std::string shortpath;
	char * homedir = getenv("HOME");
	if (info_get_cwd_full(&fullpath)) {
		if (homedir == NULL) {
			*ostr = fullpath;
			return 1;
		}
		if (strncmp(fullpath.c_str(), homedir, strlen(homedir))==0) {
			try {
				shortpath = fullpath.substr(strlen(homedir)+1);
				shortpath.insert(0,"~/");
				*ostr = shortpath;
			} catch (std::out_of_range) {
				shortpath = "~";
				*ostr = shortpath;
			}
			return 1;
		} else {
			*ostr = fullpath;
			return 1;
		}
	}
	return 0;
}

bool info_get_root() {
	int res = getuid();
	if (res != 0) {
		return false;
	} else {
		return true;
	}
}

int info_get_hostname(std::string * ostr) {
#ifdef USING_WINDOWS
	char hostname[MAX_COMPUTERNAME_LENGTH + 1];
	long unsigned int size = sizeof(hostname) / sizeof(hostname[0]);
	GetComputerName(hostname, &size);
	int result = 0;
#else
	char hostname[HOST_NAME_MAX];
	int result = gethostname(hostname, HOST_NAME_MAX);
#endif
	if (result) {
		return 0;
	} else {
		*ostr = std::string(hostname);
		return 1;
	}
}

int info_get_login(std::string * ostr) {
	char username[LOGIN_NAME_MAX];
	int result = getlogin_r(username, LOGIN_NAME_MAX);
	if (result) {
		return 0;
	} else {
		*ostr = std::string(username);
		return 1;
	}
}

int info_get_git_branch(std::string * ostr) {
	struct stat info;
	char buf[512];
	bool foundgit = false;
	if (getcwd(&buf[0],sizeof(buf)) == NULL) {
		return 0;
	}
	std::string cwd(&buf[0]);
	std::string ghead("/.git/HEAD");
	while (!foundgit) {
		if (stat((cwd+ghead).c_str(), &info) != 0) {
			size_t found;
			std::string wdsstr;
			found = cwd.find_last_of("/");
			if (found != std::string::npos) {
				cwd = cwd.substr(0,found);
			} else {
				return 0;
			}
			continue;
		} else if (info.st_mode & S_IFDIR) {
			return 0;
		} else {
			foundgit = true;
			continue;
		}
	}
	std::regex brpattern("ref: refs\\/heads\\/(.*)");
	std::smatch brmatch;
	std::ifstream ifs(cwd+ghead);
	std::string content( 
		(std::istreambuf_iterator<char>(ifs) ),
		(std::istreambuf_iterator<char>()    ) 
	);
	std::string result;
	if (std::regex_search(content, brmatch, brpattern) 
		&& brmatch.size() > 1) {
		*ostr = brmatch.str(1);
	} else {
		return 0;
	}
	return 1;
}

int info_get_hg_branch(std::string * ostr) {
	struct stat info, binfo;
	char buf[512];
	bool foundhg = false;
	if (getcwd(&buf[0],sizeof(buf)) == NULL) {
		return 0;
	}
	std::string cwd(&buf[0]);
	std::string hhead("/.hg");
	while (!foundhg) {
		if (stat(((cwd+hhead).c_str()), &info) != 0) {
			size_t found;
			std::string wdsstr;
			found = cwd.find_last_of("/");
			if (found != std::string::npos) {
				cwd = cwd.substr(0,found);
			} else {
				return 0;
			}
			continue;
		} else if (info.st_mode & S_IFDIR) {
			foundhg = true;
			continue;
		} else {
			return 0;
		}
	}
	if (stat((cwd+std::string("/.hg/branch")).c_str(), &binfo) != 0) {
		*ostr = "default";
		return 1;
	} else if (binfo.st_mode & S_IFDIR) {
		return 0;
	}
	std::ifstream ifs(cwd+std::string("/.hg/branch"));
	std::string content( 
		(std::istreambuf_iterator<char>(ifs) ),
		(std::istreambuf_iterator<char>()    ) 
	);
	content.erase(
		std::remove(
			content.begin(), 
			content.end(), 
			'\n'
			), 
		content.end()
	);
	*ostr = content;
	return 1;
}

int main(int argc, char**argv) {
	pl_manager plm;
	plm.lchar = "\uE0B0";
	plm.rchar = "\uE0B2";
	
	auto argv1 = std::string(argv[1]);
	if (argc > 0) {
		if (argv1 == std::string("0")) {
			pl_segment userandhost(15,236);
			std::string hname;
			std::string uname;
			
			if (info_get_hostname(&hname) && info_get_login(&uname)) {
				userandhost.text.append(uname);
				userandhost.text.append("@");
				userandhost.text.append(hname);
				std::transform(userandhost.text.begin(),
					userandhost.text.end(), 
					userandhost.text.begin(), 
					::tolower
				);
				pad_space(&userandhost.text);
				plm.segments_left.push_back(userandhost);
			}
			
			pl_segment currentwd(15,234);
			std::string wdstr;
			if (info_get_cwd_short(&wdstr)) {
				currentwd.text.append(wdstr);
				pad_space(&currentwd.text);
				plm.segments_left.push_back(currentwd);
			}
			
			pl_segment gitstatus(15,33);
			std::string gbranch;
			if (info_get_git_branch(&gbranch)) {
				gitstatus.text.append("\uF1D3 ");
				gitstatus.text.append(gbranch);
				pad_space(&gitstatus.text);
				plm.segments_left.push_back(gitstatus);
			}
			
			pl_segment hgstatus(15,198);
			std::string hgbranch;
			if (info_get_hg_branch(&hgbranch)) {
				hgstatus.text.append("\uF069 ");
				hgstatus.text.append(hgbranch);
				pad_space(&hgstatus.text);
				plm.segments_left.push_back(hgstatus);
			}
			
			if (info_get_root()) {
				plm.segments_left.push_back(pl_segment(15,199," $ "));
			} else {
				plm.segments_left.push_back(pl_segment(15,99," # "));
			}
			plm.render_segments_left();
		} else if (argv1 == std::string("1")) {
			if (argc >= 3) {
				int exits = atoi(argv[2]);
				if (exits != 0) {
					std::string exitstr(argv[2]);
					pad_space(&exitstr);
					plm.segments_right.push_back(pl_segment(15,9,exitstr));
				}
			}
			std::string shortpath;
			if (info_get_cwd_home(&shortpath)) {
				pl_segment homeseg(15,236);
				pad_space(&shortpath);
				homeseg.text = shortpath;
				plm.segments_right.push_back(homeseg);
			}
			
			plm.render_segments_right();
		}
	} else {
		std::cout << "Specify segment number (0 left, 1 right)" << std::endl;
	}
}