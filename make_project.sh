#!/bin/bash

# NOTE: rust naming only allows '_' as special character
_TARGET=$(echo "$1" | sed 's/ /_/g' | sed 's/:/_/g' | sed 's/\*/_/g' | sed 's/\?/_/g' | sed 's/"/_/g' | sed 's/</_/g' | sed 's/>/_/g' | sed 's/|/_/g' | sed 's/\\/_/g' | sed 's/\//_/g')
_TARGET_UPPER=$(echo $_TARGET | tr '[:lower:]' '[:upper:]' | sed 's/ /_/g')
_TARGET_LOWER=$(echo $_TARGET | tr '[:upper:]' '[:lower:]' | sed 's/ /_/g')

if [ -z "${_TARGET}" ]; then
    echo "Usage: $0 <project_name>"
    exit 1
fi

if [ -d ${_TARGET_LOWER} ]; then
    echo "Project ${_TARGET_LOWER} already exists"
    exit 1
fi

function make_root_cd() {
    mkdir ${_TARGET_LOWER}
    cd ${_TARGET_LOWER}

    touch README.md
}

function rust_make_root_cd() {
    cargo new ${_TARGET_LOWER} --lib
    cd ${_TARGET_LOWER}
}

# assumes we're already in the same directory where Cargo.toml is
function rust_make_mod() {
    touch src/${_TARGET_LOWER}.rs
    echo "mod ${_TARGET_LOWER};
" >>src/mod.rs
}
function rust_make_lib() {
    touch src/${_TARGET_LOWER}.rs
    echo "
pub fn do_something(s: &str) {
    println!(\"do_something {s}\");
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::${_TARGET_LOWER};

    #[test]
    fn it_works() {
        ${_TARGET_LOWER}::do_something(\"lib test\");
    }
}
" >>src/${_TARGET_LOWER}.rs
}

function rust_update_lib_proxy() {
    touch src/lib.rs
    echo "
pub mod ${_TARGET_LOWER};

// pub accessor methods of the library included in mod.rs
pub fn add(left: usize, right: usize) -> usize {
    ${_TARGET_LOWER}::do_something(\"from add()\");
    left + right
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        let result = add(2, 2);
        assert_eq!(result, 4);
    }

    #[test]
    fn it_works2() {
        ${_TARGET_LOWER}::do_something(\"entry point test\");
    }
}
" > src/lib.rs  # write over (not append)
}

function cpp_make_main() {
    touch src/main.cpp
}

function cpp_make_hpp() {
    touch src/lib_${_TARGET_LOWER}.hpp

    echo "// lib entry point
#define _HAS_CXX20 1
#include \"lib_${_TARGET_LOWER}.hpp\"


int main() {
    return 0;
}
" >>src/main.cpp

    echo "
#ifndef ${_TARGET_UPPER}
#define ${_TARGET_UPPER}

// My essentials that I always include:
#if _HAS_CXX20  // NOTE: Though I just need C++17 so I can use std::optional and lamdas, I believe Hackerrank allows UP TO uses C++20
#include <algorithm> // std::sort, std::transform, std::find (std::find - make sure to override operator==)
#include <array>
#include <cassert>    // assert()
#include <chrono>     // for start/end time measurement
#include <cstdint>    // std::uint16_t, etc - I'm too used to rust types...
#include <fstream>    // for reading in file
#include <functional> // lambdas!
#include <iostream>   // std::cout
#include <memory>     // std::shared_ptr, std::make_shared
#include <optional>   // a bit different from Rust Option<T> but still, useful!
#include <stack>  // commonly used when I need to convert recursive to iterative
#include <string>
#include <tuple>
#include <unordered_map> // use map if need keys to be ordered, but generally, I just need key to be hashed...
#include <unordered_set>
#include <utility> // std::pair, etc
#include <vector>
#else
// fail compiler if C++ version is less than C++20
// but without using static_assert() because it's not available until C++17
#error This code requires at least C++17
#endif // !_HAS_CXX20 || !_HAS_CXX17

using namespace std;

namespace hairev { namespace libs { 
    class ${_TARGET} {
        /// TODO: make sure to code your methods in ways so that
        /// you can copy-and-paste as-is, because most online puzzles
        /// will not allow separate files to include, etc...
        /// Hint: use lambdas within methods so that it is part of the method

    };
}}

#endif // ${_TARGET_UPPER}
" >>src/lib_${_TARGET_LOWER}.hpp
}

function make_md() {
    touch README.md
    echo "# ${_TARGET}

## Rust

## C++

" >>README.md

}

make_root_cd

pushd .
rust_make_root_cd
rust_make_mod
rust_make_lib
rust_update_lib_proxy
popd
mv ${_TARGET_LOWER} rust

# NOTE: assume both hpp and cpp resides on 'src' dir rather than 'include' and 'src'
#       because these libs are supposed to be small!
pushd .
mkdir cpp
mkdir cpp/src
cd cpp
cpp_make_main
cpp_make_hpp
popd

make_md

# assume .gitignore won't add crufts, and just do git add
git add .
