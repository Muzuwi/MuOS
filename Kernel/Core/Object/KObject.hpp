#pragma once
#include <LibGeneric/Optional.hpp>
#include <LibGeneric/String.hpp>
#include <stddef.h>
#include "Capability.hpp"
#include "Root.hpp"

namespace core::object {
	class KObject {
	public:
		explicit KObject(gen::String name)
		    : m_name(gen::move(name))
		    , m_type()
		    , m_ref_count(0)
		    , m_raw(nullptr) {}

		gen::String const& name() const { return m_name; }

		size_t ref_count() const { return m_ref_count; }

		template<Capability T>
		gen::Optional<T*> capability() {
			if(m_type != T::type()) {
				return { gen::nullopt };
			}
			return static_cast<T*>(m_raw);
		}

		template<Capability T>
		bool register_capability(T* cap) {
			//  Don't allow re-registering right now
			if(!cap || !m_type.empty()) {
				return false;
			}

			m_type = T::type();
			m_raw = cap;
			return true;
		}
	private:
		gen::String m_name;
		gen::String m_type;
		size_t m_ref_count;
		void* m_raw;
	};
}

namespace foobar {
	static inline void fo() {
		struct ISerial {
			static gen::String type() { return gen::String { "ISerial" }; }
			void foo() {
				//  foobar
			}
		} my_serial;

		auto cool_object = core::object::KObject { gen::String { "serial0" } };
		if(!cool_object.register_capability(&my_serial)) {
			//  failed registering cap for whatever reason
		}
		core::object::connect("/Serial/", cool_object);

		auto iterator = core::object::iterate("/Serial/");
		if(iterator.has_value()) {
			//  failed acquiring iterator, probably wrong path or w/e
		}

		for(core::object::KObject object : iterator) {
			//  version 1
			//			if(object.capable<ISerial>()) {
			//				auto serial = object.capability<ISerial>();
			//				//  do fun stuff with object
			//			}
			//  version 2
			if(auto serial = object.capability<ISerial>(); serial.has_value()) {
				auto f = serial.unwrap();
			}
		}
	}
}
