section .text

%macro def_entry_exception 1
global _exception_entry_%{1:1}
_exception_entry_%{1:1}:
._stub:
    jmp ._stub
%endmacro

%macro def_entry_exception_errorcode 1
global _exception_entry_%{1:1}
_exception_entry_%{1:1}:
._stub:
    jmp ._stub
%endmacro


def_entry_exception 0
def_entry_exception 1
def_entry_exception 2
def_entry_exception 3
def_entry_exception 4
def_entry_exception 5
def_entry_exception 6
def_entry_exception 7
def_entry_exception_errorcode 8
def_entry_exception 9
def_entry_exception_errorcode 10
def_entry_exception_errorcode 11
def_entry_exception_errorcode 12
def_entry_exception_errorcode 13
def_entry_exception_errorcode 14
def_entry_exception 15
def_entry_exception 16
def_entry_exception_errorcode 17
def_entry_exception 18
def_entry_exception 19
def_entry_exception 20
def_entry_exception 21
def_entry_exception 22
def_entry_exception 23
def_entry_exception 24
def_entry_exception 25
def_entry_exception 26
def_entry_exception 27
def_entry_exception 28
def_entry_exception 29
def_entry_exception_errorcode 30
def_entry_exception 31