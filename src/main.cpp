#include "main.hpp"
#include "config.hpp"
#include "config_parser.hpp"
#include "git_info.hpp"

#include <cstdio>
#include <gtkmm/application.h>
#include <filesystem>
#include <iostream>
#include <dlfcn.h>

void handle_signal(int signum) {
	sysmenu_handle_signal_ptr(win, signum);
}

void usage() {
	std::printf("usage:\n");
	std::printf("  sysmenu [argument...]:\n\n");
	std::printf("arguments:\n");
	std::printf("  -S	Hide the program on launch\n");
	std::printf("  -s	Hide the search bar\n");
	std::printf("  -i	Set launcher icon size\n");
	std::printf("  -I	Set dock icon size\n");
	std::printf("  -u	Show name under icon\n");
	std::printf("  -b	Show scroll bars\n");
	std::printf("  -n	Max name length\n");
	std::printf("  -a	Set anchors\n");
	std::printf("  -W	Set window width\n");
	std::printf("  -H	Set window Height\n");
	#ifdef FEATURE_SCRIPTING
	std::printf("  -M	Set launcher margins\n");
	std::printf("  -m	Set primary monitor\n");
	std::printf("  -p	Set placeholder text\n");
	std::printf("  -P	Items per row\n");
	std::printf("  -L	Disable use of layer shell\n");
	std::printf("  -d	dmenu emulation\n");
	#else
	std::printf("  -m	Set launcher margins\n");
	std::printf("  -M	Set primary monitor\n");
	std::printf("  -P	Items per row\n");
	std::printf("  -l	Disable use of layer shell\n");
	#endif
	std::printf("  -D	Set dock items\n");
	std::printf("  -v	Prints version info\n");
	std::printf("  -h	Show this help message\n");
}

void load_libsysmenu() {
	void* handle = dlopen("libsysmenu.so", RTLD_LAZY);
	if (!handle) {
		std::cerr << "Cannot open library: " << dlerror() << '\n';
		exit(1);
	}

	sysmenu_create_ptr = (sysmenu_create_func)dlsym(handle, "sysmenu_create");
	sysmenu_handle_signal_ptr = (sysmenu_handle_signal_func)dlsym(handle, "sysmenu_signal");

	if (!sysmenu_create_ptr || !sysmenu_handle_signal_ptr) {
		std::cerr << "Cannot load symbols: " << dlerror() << '\n';
		dlclose(handle);
		exit(1);
	}
}

int main(int argc, char* argv[]) {
	// Load the config
	#ifdef CONFIG_FILE
	std::string config_path;
	std::map<std::string, std::map<std::string, std::string>> config;
	std::map<std::string, std::map<std::string, std::string>> config_usr;

	bool cfg_sys = std::filesystem::exists("/usr/share/sys64/menu/config.conf");
	bool cfg_sys_local = std::filesystem::exists("/usr/local/share/sys64/menu/config.conf");
	bool cfg_usr = std::filesystem::exists(std::string(getenv("HOME")) + "/.config/sys64/menu/config.conf");

	// Load default config
	if (cfg_sys)
		config_path = "/usr/share/sys64/menu/config.conf";
	else if (cfg_sys_local)
		config_path = "/usr/local/share/sys64/menu/config.conf";
	else
		std::fprintf(stderr, "No default config found, Things will get funky!\n");

	config = config_parser(config_path).data;

	// Load user config
	if (cfg_usr)
		config_path = std::string(getenv("HOME")) + "/.config/sys64/menu/config.conf";
	else
		std::fprintf(stderr, "No user config found\n");

	config_usr = config_parser(config_path).data;

	// Merge configs
	for (const auto& [key, nested_map] : config_usr)
		for (const auto& [inner_key, inner_value] : nested_map)
			config[key][inner_key] = inner_value;

	// Sanity check
	if (!(cfg_sys || cfg_sys_local || cfg_usr)) {
		std::fprintf(stderr, "No config available, Something ain't right here.");
		return 1;
	}
	#endif

	// Read launch arguments
	#ifdef CONFIG_RUNTIME

	#ifdef FEATURE_SCRIPTING
	bool dmenuarg=false;
	#endif

	while (true) {
		switch(getopt(argc, argv, "Ssi:I:m:ubn:p:a:W:H:M:l:D:vhLP:dw:")) {
			case 'S':
				config["main"]["start-hidden"] = "true";
				continue;

			case 's':
				config["main"]["searchbar"] = "false";
				continue;

			case 'i':
				config["main"]["icon-size"] = optarg;
				continue;

			case 'I':
				config["main"]["dock-icon-size"] = optarg;
				continue;

			case 'u':
				config["main"]["name-under-icon"] = "true";
				continue;

			case 'b':
				config["main"]["scroll-bars"] = "true";
				continue;

			case 'n':
				config["main"]["name-length"] = optarg;
				continue;
			case 'a':
				config["main"]["anchors"] = optarg;
				continue;

			case 'W':
				config["main"]["width"] = optarg;
				continue;

			case 'H':
				config["main"]["height"] = optarg;
				continue;

			case 'D':
				config["main"]["dock-items"] = optarg;
				config["main"]["layer-shell"] = "true";
				config["main"]["anchors"] = "top right bottom left";
				continue;

			// Assumption: who enables scripting will prefer dmenu-compatible CLI
			// who doesn't prefers backwards-compatible CLI
			#ifdef FEATURE_SCRIPTING

			case 'M':
				config["main"]["app-margin"] = optarg;
				continue;

			case 'P':
				config["main"]["items-per-row"] = optarg;
				continue;

			case 'p':
				config["main"]["prompt"] = optarg;
				continue;


			case 'm':
				config["main"]["monitor"] = optarg;
				continue;

			case 'd':
				config["main"]["dmenu"] = "true";
				continue;

			case 'L':
				config["main"]["layer-shell"] = "false";
				continue;

			// -l sets the number of lines and -w window embedding in dmenu scripts
			// there is no trivial way to implement it, but ignoring it is an option
			case 'l':
			case 'w':
				dmenuarg=true;
				continue;

			#else

			case 'm':
				config["main"]["app-margin"] = optarg;
				continue;

			case 'p':
				config["main"]["items-per-row"] = optarg;
				continue;

			case 'M':
				config["main"]["monitor"] = optarg;
				continue;

			case 'l':
				config["main"]["layer-shell"] = "false";
				continue;

			#endif

			case 'v':
				std::printf("Commit: %s\nDate: %s\n", GIT_COMMIT_MESSAGE, GIT_COMMIT_DATE);
				return 0;

			case 'h':
			default :
				usage();
				return 0;

			case -1:
				break;
			}

			break;
	}

	#ifdef FEATURE_SCRIPTING
	if ( config["main"]["dmenu"] == "true" ) {
		config["main"]["start-hidden"] = "false";
	} else if ( dmenuarg ) {
		std::printf("Dmenu args used, but not in dmenu mode\n");
		usage();
		return 0;
	}
	#endif
	#endif

	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create("funky.sys64.sysmenu");
	app->hold();

	load_libsysmenu();
	win = sysmenu_create_ptr(config);

	#ifdef FEATURE_SCRIPTING
	// Catch signals if not in dmenu mode
	if (config["main"]["dmenu"] != "true") {
		signal(SIGUSR1, handle_signal);
		signal(SIGUSR2, handle_signal);
		signal(SIGRTMIN, handle_signal);
	}
	#else
	signal(SIGUSR1, handle_signal);
	signal(SIGUSR2, handle_signal);
	signal(SIGRTMIN, handle_signal);
	#endif

	return app->run();
}
