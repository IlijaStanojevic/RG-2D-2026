#version 330 core
out vec4 FragColor;

in vec3 chNormal;
in vec3 chFragPos;
in vec2 chTex;

uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform vec3 uLightColor;

uniform sampler2D uTex0;

void main()
{
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * uLightColor;
  	
    // diffuse 
    vec3 norm = normalize(chNormal);
    vec3 lightDir = normalize(uLightPos - chFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;
    
    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(uViewPos - chFragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * uLightColor; 

    vec4 tex = texture(uTex0, chTex);
    FragColor = tex * vec4(ambient + diffuse + specular, 1.0);
}
