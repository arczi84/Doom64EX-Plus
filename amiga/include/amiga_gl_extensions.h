/* MiniGL single-texture fallback; no desktop GL proc ABI is used. */
extern boolean has_GL_ARB_multitexture;
#define GL_ARB_multitexture_Define() boolean has_GL_ARB_multitexture = false
#define GL_ARB_multitexture_Init() (has_GL_ARB_multitexture = false)
extern boolean has_GL_EXT_compiled_vertex_array;
#define GL_EXT_compiled_vertex_array_Define() boolean has_GL_EXT_compiled_vertex_array = false
#define GL_EXT_compiled_vertex_array_Init() (has_GL_EXT_compiled_vertex_array = false)
extern boolean has_GL_ARB_texture_env_combine;
#define GL_ARB_texture_env_combine_Define() boolean has_GL_ARB_texture_env_combine = false
#define GL_ARB_texture_env_combine_Init() (has_GL_ARB_texture_env_combine = false)
extern boolean has_GL_EXT_texture_env_combine;
#define GL_EXT_texture_env_combine_Define() boolean has_GL_EXT_texture_env_combine = false
#define GL_EXT_texture_env_combine_Init() (has_GL_EXT_texture_env_combine = false)
extern boolean has_GL_EXT_texture_filter_anisotropic;
#define GL_EXT_texture_filter_anisotropic_Define() boolean has_GL_EXT_texture_filter_anisotropic = false
#define GL_EXT_texture_filter_anisotropic_Init() (has_GL_EXT_texture_filter_anisotropic = false)
#define dglActiveTextureARB(t) glActiveTextureARB(t)
