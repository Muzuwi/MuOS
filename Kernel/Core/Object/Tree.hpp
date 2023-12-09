#pragma once
#include <Structs/KFunction.hpp>
#include "Core/Error/Error.hpp"
#include "Core/Object/Object.hpp"

namespace core::obj {
	using FindObjectCallback = KFunction<void(KObject*)>;

	core::Error attach(KObject*);
	core::Error for_each_object_of_type(ObjectType, FindObjectCallback);
}
