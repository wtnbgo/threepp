
#ifdef USE_MORPHTARGETS

	uniform float morphTargetBaseInfluence;
	uniform float morphTargetInfluences[ MORPHTARGETS_COUNT ];

	// 全モーフをテクスチャに詰め、頂点ごとに texelFetch で参照する方式。
	// 旧来の 8 attribute スロット制限を撤廃し、任意数のモーフを同時適用する。
	uniform highp sampler2D morphTargetsTexture;
	uniform int morphTargetsTextureWidth;   // テクスチャ幅 (texel 数)
	uniform int morphTargetsTexelsPerMorph;  // = 頂点数 * MORPHTARGETS_STRIDE

	vec3 getMorph( const in int vertexIndex, const in int morphTargetIndex, const in int offset ) {

		int texelIndex = morphTargetIndex * morphTargetsTexelsPerMorph + vertexIndex * MORPHTARGETS_STRIDE + offset;
		int y = texelIndex / morphTargetsTextureWidth;
		int x = texelIndex - y * morphTargetsTextureWidth;
		return texelFetch( morphTargetsTexture, ivec2( x, y ), 0 ).xyz;

	}

#endif
