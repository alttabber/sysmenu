#pragma once
#ifdef FEATURE_SCRIPTING

class dmenuentry {
	public:
		Glib::ustring content;
		Glib::ustring icon;
		void unreference(){}
};

#endif
