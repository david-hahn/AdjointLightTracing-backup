#pragma once

#include <tamashii/public.hpp>
#include <tamashii/bindings/bindings.hpp>

T_BEGIN_PYTHON_NAMESPACE

template<typename T>
using Vector2 = nb::ndarray<T, nb::shape<2>, nb::c_contig, nb::device::cpu, nb::numpy>;
template<typename T>
using Vector3 = nb::ndarray<T, nb::shape<3>, nb::c_contig, nb::device::cpu, nb::numpy>;
template<typename T>
using Vector4 = nb::ndarray<T, nb::shape<4>, nb::c_contig, nb::device::cpu, nb::numpy>;

template<typename T>
using Array = nb::ndarray<T, nb::shape<nb::any>, nb::c_contig, nb::device::cpu, nb::numpy>;

template<typename T>
using Image = nb::ndarray<T, nb::shape<nb::any, nb::any, 3>, nb::c_contig, nb::device::cpu, nb::numpy>;

template<typename T>
using Matrix4x4 = nb::ndarray<T, nb::shape<4, 4>, nb::c_contig, nb::device::cpu, nb::numpy>;

template<typename T>
glm::mat<4, 4, T, glm::defaultp> toGlmMat4x4(const Matrix4x4<T>& m) {
	return glm::make_mat4(m.data());
}

template<typename T>
glm::vec<3, T, glm::defaultp> toGlmVec3(const Vector3<T>& m) {
    return glm::make_vec3(m.data());
}

template <typename... Args>
void inspect(nb::ndarray<Args...> a) {
    

    printf("Tensor data pointer : %p\n", a.data());
    printf("Tensor dimension : %zu\n", a.ndim());
    for (size_t i = 0; i < a.ndim(); ++i) {
        printf("Tensor dimension [%zu] : %zu\n", i, a.shape(i));
        printf("Tensor stride    [%zu] : %zd\n", i, a.stride(i));
    }
    printf("Device ID = %u (cpu=%i, cuda=%i)\n", a.device_id(),
        static_cast<int>(a.device_type() == nb::device::cpu::value),
        static_cast<int>(a.device_type() == nb::device::cuda::value)
    );
    printf("Tensor dtype: int16=%i, uint32=%i, float32=%i\n",
        a.dtype() == nb::dtype<int16_t>(),
        a.dtype() == nb::dtype<uint32_t>(),
        a.dtype() == nb::dtype<float>()
    );
}

T_END_PYTHON_NAMESPACE
