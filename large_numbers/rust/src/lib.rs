
pub mod large_numbers;

// pub accessor methods of the library included in mod.rs
pub fn add(left: usize, right: usize) -> usize {
    large_numbers::do_something("from add()");
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
        large_numbers::do_something("entry point test");
    }
}

