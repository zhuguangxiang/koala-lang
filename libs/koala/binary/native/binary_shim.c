/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Module init for the koala/binary native module.
 *
 * One C file per Koala class (see binary_shim.h):
 *   - byteorder.c: ByteOrder (behind BigEndian / LittleEndian globals)
 *   - reader.c:    BinReader
 *   - writer.c:    BinWriter
 */

#include "binary_shim.h"

/// Entry point called by the runtime after `dlopen("libkoala_binary.so")`.
///
/// The name must be `<last segment of the package path>_native_lib_init`,
/// so `koala/binary` becomes `binary_native_lib_init`.
void binary_native_lib_init(NativeLib *lib)
{
    binary_reg_byteorder(lib);
    binary_reg_reader(lib);
    binary_reg_writer(lib);
}
