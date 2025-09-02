/**************************************************************************/
/*  ruby_script.h                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "ruby_gc_handle.h"
#include "ruby_gd/gd_ruby.h"

#include "core/doc_data.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/script_language.h"
#include "core/templates/self_list.h"

#ifdef TOOLS_ENABLED
#include "editor/plugins/editor_plugin.h"
#endif

class RubyScript;
class RubyInstance;
class RubyLanguage;

// Why TScriptLanguage unused? Also why not handle nullptr in case 
// dynamic_cast fails?
template <typename TScriptInstance, typename TScriptLanguage>
TScriptInstance *cast_script_instance(ScriptInstance *p_inst) {
	return dynamic_cast<TScriptInstance *>(p_inst);
}

#define CAST_RUBY_INSTANCE(m_inst) (cast_script_instance<RubyInstance, RubyLanguage>(m_inst))

/*MAI
    The GDCLASS macro is used to define a class in the Godot Engine that inherits from another class and integrates with the ClassDB system for runtime type information, property management, and method binding. It ensures proper initialization, class registration, and compatibility handling while providing utilities for class-specific property and method management.
*/
class RubyScript : public Script {
	GDCLASS(RubyScript, Script);

	friend class RubyInstance;
	friend class RubyLanguage;

    public:
	struct TypeInfo {
		/**
		 * Name of the Ruby class.
		 */
		String class_name;

        /*MAI
            In the context of Godot's scripting system, native_base_name (a StringName field in the TypeInfo struct) refers to the name of the built-in Godot class that the script is designed to extend or derive from. This is a key concept in Godot's object-oriented scripting model, where scripts attach to nodes and define custom behavior by inheriting from (or "extending") a base Godot class.
        */
		/**
		 * Name of the native class this script derives from.
		 */
		StringName native_base_name;

		/**
		 * Path to the icon that will be used for this class by the editor.
		 */
		String icon_path;

		/**
		 * Script is marked as tool and runs in the editor.
		 */
		bool is_tool = false;

		/**
		 * Script is marked as global class and will be registered in the editor.
		 * Registered classes can be created using certain editor dialogs and
		 * can be referenced by name from other languages that support the feature.
		 */
		bool is_global_class = false;

		/**
		 * Script is declared abstract.
		 */
		bool is_abstract = false;

        /*MAI
            Ruby doesn't really have/need generics, as it is dynamically typed?
            TODO: How to handle?
        */
		/**
		 * The Ruby type that corresponds to this script is a constructed generic type.
		 * E.g.: `Dictionary<int, string>`
		 */
		bool is_constructed_generic_type = false;

        /*MAI
            Ruby doesn't really have/need generics, as it is dynamically typed?
            TODO: How to handle?
        */
		/**
		 * The Ruby type that corresponds to this script is a generic type definition.
		 * E.g.: `Dictionary<,>`
		 */
		bool is_generic_type_definition = false;

        /*MAI
            Ruby doesn't really have/need generics, as it is dynamically typed?
            TODO: How to handle?
        */
		/**
		 * The Ruby type that corresponds to this script contains generic type parameters,
		 * regardless of whether the type parameters are bound or not.
		 */
		bool is_generic() const {
			return is_constructed_generic_type || is_generic_type_definition;
		}

        /*MAI
            Ruby doesn't really have/need generics, as it is dynamically typed?
            TODO: How to handle? Also change CSharpScript below.
        */
		/**
		 * Check if the script can be instantiated.
		 * Ruby types can't be instantiated if they are abstract or contain generic?
		 * type parameters, but a CSharpScript is still created for them.
		 */
		bool can_instantiate() const {
			return !is_abstract && !is_generic_type_definition;
		}
	};

    private:
	/**
	 * Contains the Ruby type information for this script.
	 */
	TypeInfo type_info;

    /*MAI
        Below writes "Scripts are valid when the corresponding Ruby class is found"; When is this "search" being made?
        TODO : Research
    */
	/**
	 * Scripts are valid when the corresponding Ruby class is found and used
	 * to extract the script info using the [update_script_class_info] method.
	 */
	bool valid = false;
	/**
	 * Scripts extract info from the Ruby class in the reload methods but,
	 * if the reload is not invalidated, then the current extracted info
	 * is still valid and there's no need to reload again.
	 */
	bool reload_invalidated = false;

	/**
	 * Base script that this script derives from, or null if it derives from a
	 * native Godot class.
	 */
	Ref<RubyScript> base_script;

	HashSet<Object *> instances;

// TODO : Add/Check existence requirements for the new "GD_RUBY_HOT_RELOAD"
#ifdef GD_RUBY_HOT_RELOAD
	struct StateBackup {
		// TODO
		// Replace with buffer containing the serialized state of managed scripts.
		// Keep variant state backup to use only with script instance placeholders.
		List<Pair<StringName, Variant>> properties;
		Dictionary event_signals;
	};

	HashSet<ObjectID> pending_reload_instances;
	RBMap<ObjectID, StateBackup> pending_reload_state;

	bool was_tool_before_reload = false;
	HashSet<ObjectID> pending_replace_placeholders;
#endif

	/**
	 * Script source code.
	 */
	String source;

	SelfList<RubyScript> script_list = this;

    /*MAI
        The rpc_config is a Dictionary object defined within the RubyScript namespace. It likely serves as a container for key-value pairs related to the configuration of RPC (Remote Procedure Call) functionality in the context of the RubyScript module.
    */
	Dictionary rpc_config;

	struct EventSignalInfo {
		StringName name; // MethodInfo stores a string...
		MethodInfo method_info;
	};

	struct RubyMethodInfo {
		StringName name; // MethodInfo stores a string...
		MethodInfo method_info;
	};

	Vector<EventSignalInfo> event_signals;
	Vector<RubyMethodInfo> methods;

#ifdef TOOLS_ENABLED
	List<PropertyInfo> exported_members_cache; // members_cache
	HashMap<StringName, Variant> exported_members_defval_cache; // member_default_values_cache
	HashSet<PlaceHolderScriptInstance *> placeholders;
	bool source_changed_cache = false;
	bool placeholder_fallback_enabled = false;
	bool exports_invalidated = true;
	void _update_exports_values(HashMap<StringName, Variant> &values, List<PropertyInfo> &propnames);
	void _placeholder_erased(PlaceHolderScriptInstance *p_placeholder) override;
#endif

#if defined(TOOLS_ENABLED) || defined(DEBUG_ENABLED)
	HashSet<StringName> exported_members_names;
#endif

	HashMap<StringName, PropertyInfo> member_info;

	void _clear();

	static void GD_CLR_STDCALL _add_property_info_list_callback(RubyScript *p_script, const String *p_current_class_name, void *p_props, int32_t p_count);
#ifdef TOOLS_ENABLED
	static void GD_CLR_STDCALL _add_property_default_values_callback(RubyScriptp_script, void *p_def_vals, int32_t p_count);
#endif
	bool _update_exports(PlaceHolderScriptInstance *p_instance_to_update = nullptr);

	RubyInstance *_create_instance(const Variant **p_args, int p_argcount, Object *p_owner, bool p_is_ref_counted, Callable::CallError &r_error);
	/*MAI
		_new is interesting because it returns a Variant, not a RubyInstance*.
		It is used in the context of creating a new instance of a Ruby script within the Godot engine.
	*/
	Variant _new(const Variant **p_args, int p_argcount, Callable::CallError &r_error);

	// Do not use unless you know what you are doing
	static void update_script_class_info(Ref<RubyScript> p_script);

	void _get_script_signal_list(List<MethodInfo> *r_signals, bool p_include_base) const;
};