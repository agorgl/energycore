//
// blending.glsl
//

// Normal blending (base normal and details normal)
// http://blog.selfshadow.com/publications/blending-in-detail/

vec3 blend_normals(vec3 n1, vec3 n2)
{
    mat3 n_basis = mat3(
        vec3(n1.z, n1.y, -n1.x), // +90 degree rotation around y axis
        vec3(n1.x, n1.z, -n1.y), // -90 degree rotation around x axis
        vec3(n1.x, n1.y,  n1.z)
    );
    return normalize(n2.x * n_basis[0] + n2.y * n_basis[1] + n2.z * n_basis[2]);
}

vec3 blend_multiply(vec3 a, vec3 b)
{
    return a * b;
}
