/*----------------------------------------------------------------------------*\
|* This file is part of the koala project, under the MIT License.             *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

package main

import (
	"fmt"
	"unsafe"
)

const (
	_KLVMTypeInt8  = 1
	_KLVMTypeInt16 = 2
)

// KLVMType type
type KLVMType struct {
	kind int8
	name string
}

// KVLMValue value
type KVLMValue struct {
	_kind int8
	KLVMType
}

// KLVMType2 go-lint
type KLVMType2 interface {
}

// KLVMTypeInt8 xx
type KLVMTypeInt8 int8

type TokenType uint16

const (
	KEYWORD TokenType = iota
	IDENTIFIER
	LBRACKET
	RBRACKET
	INT
)

type Token struct {
	Type   TokenType
	Lexeme string
}

type IntegerConstant struct {
	Token *Token
	Value uint64
}

type T struct {
	A uint32
	B int16
}

const sizeOfT = unsafe.Sizeof(T{})

func main() {
	var k KLVMType = KLVMType{kind: _KLVMTypeInt16}
	k.name = "world"
	v := KVLMValue{_kind: 18, KLVMType: KLVMType{_KLVMTypeInt16, "hello"}}
	v.KLVMType = KLVMType{kind: _KLVMTypeInt16}

	var v2 KLVMTypeInt8 = 100
	var t KLVMType2 = v2
	switch t.(type) {
	case KLVMTypeInt8:
		fmt.Println(t)
	}
	fmt.Println(v)

	var tt Token = Token{Type: 100}
	fmt.Println(tt)

	t1 := T{123, -321}
	fmt.Println(t1)

	data := (*[8]byte)(unsafe.Pointer(&t1))
	fmt.Println(data)

	t2 := *(*int16)(unsafe.Pointer(&data[4]))
	fmt.Println(t2)
}
