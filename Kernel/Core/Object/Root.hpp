#pragma once
#include <LibGeneric/String.hpp>
#include "KObject.hpp"

namespace core::object {
	///  Connect a kernel object to a specific base path in the kernel object tree
	///   @param path 	Base path of the object. This is currently an identifier, i.e
	///					a kernel subsystem
	///   @param object	Object to register
	///	  @returns Whether the object was registered successfully.
	bool connect(gen::String const& path, KObject object);

	///  Disconnect an object
	bool disconnect(gen::String const& object_path);
}

//
//  /
//  |- /Serial/
//  |-        /0
//  |- /IRQ/
//
//
//
//
//
//
//




