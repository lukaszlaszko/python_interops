import module
import numpy as np
from hamcrest import *
from numba import jit


def test_create_circular_buffer():
    c_array = module.circular_buffer_case.CircularArray(10)
    assert_that(c_array.size, is_(0))

    c_array.append(7);
    c_array.append(5);

    n_array = np.array(c_array.to_memoryview(), copy = False)
    assert_that(len(n_array), 2)
    assert_that(n_array[0], is_(7))
    assert_that(n_array[1], is_(5))


def test_create_circular_buffer_roll():
    c_array = module.circular_buffer_case.CircularArray(10)

    for i in range(c_array.capacity + 8):
        c_array.append(i)
        assert_that(c_array.tail, is_(i))

    n_array = np.array(c_array.to_memoryview(), copy = False)
    assert_that(n_array[0], 3)
    assert_that(n_array[1023], 1031)

    c_array.pop()
    n_array = np.array(c_array.to_memoryview(), copy = False)
    assert_that(n_array[0], 4)
    assert_that(n_array[1022], 1031)


def test_create_circular_buffer_array_attribute():
    c_array = module.circular_buffer_case.CircularArray(10)

    for i in range(c_array.capacity + 8):
        c_array.append(i)
        assert_that(c_array.tail, is_(i))

    n_array = c_array.array
    assert_that(n_array[0], 3)

    n_array[0] = 5000
    assert_that(c_array.head, 5000)