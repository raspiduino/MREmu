#pragma once
#include "App.h"
#include <mutex>
#include <filesystem>
#include <queue>

namespace fs = std::filesystem;

struct launch_el {
	fs::path path; 
	bool local;
};

struct keyboard_event_el {
	int event; 
	int keycode;
};

struct message_event_el {
	int phandle;
	unsigned int msg_id;
	int wparam;
	int lparam;
	int phandle_sender;
};

class AppManager {
	std::queue<launch_el> launch_queue;
	std::mutex launch_queue_mutex;


	std::queue<keyboard_event_el> keyboard_events_queue;
	std::mutex keyboard_events_queue_mutex;

	std::queue<message_event_el> message_events_queue;
	std::mutex message_events_queue_mutex;
public:
	std::vector<App> apps;
	int active_app_id = -1;
	int current_work_app_id = -1;

	void add_app_for_launch(fs::path path, bool local);
	void launch_apps();

	void add_keyboard_event(int event, int keycode);
	void process_keyboard_events();

	void add_message_event(int phandle, unsigned int msg_id, 
		int wparam, int lparam, int phandle_sender);
	void process_message_events();

	App* get_active_app();
	App* get_current_work_app_id();
};