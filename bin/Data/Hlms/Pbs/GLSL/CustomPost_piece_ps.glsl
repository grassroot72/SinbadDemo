
@piece( custom_ps_posExecution )
@property( !hlms_shadowcaster )
//Fog

    // or better "length( inPs.pos )" for a more accurate distance
    // float fogDistance = length( gl_FragCoord.z/gl_FragCoord.w );
    float fogDistance = length( inPs.pos );
    
    // (fogParams.y = fogEnd), (fogParams.x = fogStart), (fogParams.z = fogDensity)
    float fogFactor = 0;
    
    // linear fog
    //fogFactor = (passBuf.fogParams.y - fogDistance) / (passBuf.fogParams.y - passBuf.fogParams.x);
    
    // exponent fog
    // fogFactor = 1.0 / exp( fogDistance * passBuf.fogParams.z );
    
    // squared exponent fog
    fogFactor = 1.0 / exp( ( fogDistance*passBuf.fogParams.z ) * ( fogDistance*passBuf.fogParams.z ) );
    
    fogFactor = 1.0 - clamp( fogFactor, 0.0, 1.0 );
    
    vec3 fogColor = vec3( passBuf.fogColor.xyz );
    
    outColour.xyz = mix( finalColour, fogColor, fogFactor );
	
@end
@end