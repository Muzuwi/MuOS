#pragma once
#include <LibGeneric/Move.hpp>
#include <LibGeneric/String.hpp>
#include <Structs/KFunction.hpp>
#include <SystemTypes.hpp>

namespace core::obj {
	enum class ObjectType : uint64 {
		BlockDevice = 0,
	};

	class KObject {
	public:
		constexpr KObject(ObjectType type, gen::String name)
		    : m_type(type)
		    , m_name(gen::move(name)) {}

		[[nodiscard]] constexpr ObjectType type() const { return m_type; }

		[[nodiscard]] constexpr gen::String const& name() const { return m_name; }
	private:
		const ObjectType m_type;
		const gen::String m_name;
	};
}
