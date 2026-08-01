/*
 * glad/glad.h — GLES すり替えシム (krkrthreepp)
 *
 * threepp は元々デスクトップ OpenGL (glad 0.1, gl=4.1 compatibility) を前提に
 * <glad/glad.h> を取り込んでいた。しかし吉里吉里プラグインとして動かす際に
 * ホスト(吉里吉里/ANGLE)から与えられるのは OpenGL ES 3.x コンテキストであり、
 * デスクトップ GL 専用のエントリポイントは eglGetProcAddress で NULL 解決され、
 * 呼び出すと即クラッシュする(EIP=0)。
 *
 * そこで glad を glad2 の GLES ローダ (glad/gles2.h + gles2.c) に置き換え、
 * この glad.h は「gles2.h を取り込み、threepp が使うデスクトップ GL 専用の
 * 少数のシンボルを GLES 相当へマップする」薄いシムとして残す。これにより
 * threepp 本体の #include <glad/glad.h> は無改造のまま GLES ベースになる。
 *
 * ローダは threepp::initGlad()/loadGlad() (utils/LoadGlad.cpp) が
 * gladLoadGLES2() で行う。
 */
#ifndef THREEPP_GLAD_GLES_SHIM_H
#define THREEPP_GLAD_GLES_SHIM_H

#include <glad/gles2.h>

/* --- デスクトップ GL 専用トークンの補完 -----------------------------------
 * GLES3 core には無いが threepp のフォーマット表 (GLUtils.hpp) 等が名前を
 * 参照するため、コンパイルを通すために値を定義する。GL_BGRA は ANGLE が
 * EXT_texture_format_BGRA8888 (= GL_BGRA_EXT, 同値 0x80E1) で受け付ける。
 * GL_BGR は GLES に相当が無く、実行時にこの経路を通さない前提のダミー。
 */
#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

/* MSAA を glEnable(GL_MULTISAMPLE) で切り替えるのはデスクトップ GL の作法。
 * GLES では FBO のサンプル数で決まり、このトークンは無い。値だけ定義して
 * おき、実際の glEnable 呼び出しは THREEPP_GLES ガードで抑止する
 * (Canvas_generic.cpp 参照)。
 */
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif
/* デスクトップの点サイズ有効化トークン。GLES では常時有効なので値のみ定義
 * (実 glEnable は THREEPP_GLES ガードで抑止)。 */
#ifndef GL_PROGRAM_POINT_SIZE
#define GL_PROGRAM_POINT_SIZE 0x8642
#endif

/* デスクトップの glClearDepth(double) は GLES に無い。glClearDepthf へ委譲。 */
#ifndef glClearDepth
#define glClearDepth(d) glClearDepthf((GLfloat)(d))
#endif

/* glad 0.1 は要求した拡張の存在を #define <名前> 1 で表していた。threepp の
 * GLCapabilities はこれをコンパイル時 bool として参照する。float テクスチャは
 * GLES3 では core なので 1 (常に有効) を与える。 */
#ifndef GL_ARB_texture_float
#define GL_ARB_texture_float 1
#endif

#endif /* THREEPP_GLAD_GLES_SHIM_H */
