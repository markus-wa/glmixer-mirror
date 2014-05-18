


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////


void FreeFrameShadertoy::setFragmentProgramCode(const char *code)
{
    // free  previous string
    if (fragmentShaderCode)
        free(fragmentShaderCode);

    // allocate, fill and terminate string
    fragmentShaderCode = (char *) malloc(sizeof(char)*(strlen(code)+1));
    strncpy(fragmentShaderCode, code, strlen(code));
    fragmentShaderCode[strlen(code)] = '\0';

    // inform update that code has changed
    code_changed = true;
}


char *FreeFrameShadertoy::getFragmentProgramCode()
{
    return fragmentShaderCode;
}

char *FreeFrameShadertoy::getFragmentProgramLogs()
{
    return infoLog;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameShadertoy::InitGL(const FFGLViewportStruct *vp)
#else
// FFGL 1.6
FFResult FreeFrameShadertoy::InitGL(const FFGLViewportStruct *vp)
#endif
{
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = vp->width;
    viewport.height = vp->height;

    glewInit();
    if (!GLEW_VERSION_2_0) {
        fprintf(stderr, "OpenGL 2.0 not supported. Cannot use shadertoy plugin.\n");
        return FF_FAIL;
    }

    // create a texture for FBO
    glGenTextures(1,&textureFrameBufferObject.Handle);
    textureFrameBufferObject.Width = viewport.width;
    textureFrameBufferObject.Height = viewport.height;
    glBindTexture(GL_TEXTURE_2D, textureFrameBufferObject.Handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, textureFrameBufferObject.Width, textureFrameBufferObject.Height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // attach texture to FBO
    glGenFramebuffers(1, &frameBufferObject);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObject);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureFrameBufferObject.Handle, 0);


    // return to default state
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return FF_SUCCESS;
}


#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameShadertoy::DeInitGL()
#else
// FFGL 1.6
FFResult FreeFrameShadertoy::DeInitGL()
#endif
{

    if (textureFrameBufferObject.Handle) glDeleteTextures(1, &textureFrameBufferObject.Handle);
    if (frameBufferObject) glDeleteFramebuffers( 1, &frameBufferObject );
    if (fragmentShader) glDeleteShader(fragmentShader);
    if (shaderProgram)  glDeleteProgram(shaderProgram);

    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD   FreeFrameShadertoy::SetTime(double time)
#else
// FFGL 1.6
FFResult FreeFrameShadertoy::SetTime(double time)
#endif
{
    m_curTime = time;
    return FF_SUCCESS;
}

void drawQuad( FFGLViewportStruct vp, FFGLTextureStruct texture)
{
    // bind the texture to apply
    glBindTexture(GL_TEXTURE_2D, texture.Handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    //modulate texture colors with white (just show
    //the texture colors as they are)
    glColor4f(1.f, 1.f, 1.f, 1.f);

    // setup display
    glViewport(vp.x,vp.y,vp.width,vp.height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_QUADS);
    //lower left
    glTexCoord2d(0.0, 0.0);
    glVertex2f(-1,-1);

    //upper left
    glTexCoord2d(0.0, 1.0);
    glVertex2f(-1,1);

    //upper right
    glTexCoord2d(1.0, 1.0);
    glVertex2f(1,1);

    //lower right
    glTexCoord2d(1.0, 0.0);
    glVertex2f(1,-1);
    glEnd();


}


#ifdef FF_FAIL
// FFGL 1.5
DWORD	FreeFrameShadertoy::ProcessOpenGL(ProcessOpenGLStruct* pGL)
#else
// FFGL 1.6
FFResult FreeFrameShadertoy::ProcessOpenGL(ProcessOpenGLStruct *pGL)
#endif
{
    if (!pGL)
        return FF_FAIL;

    if (pGL->numInputTextures<1)
        return FF_FAIL;

    if (pGL->inputTextures[0]==NULL)
        return FF_FAIL;

    FFGLTextureStruct &Texture = *(pGL->inputTextures[0]);

    //enable texturemapping
    glEnable(GL_TEXTURE_2D);

    // no depth test
    glDisable(GL_DEPTH_TEST);

    // new code
    if(code_changed) {

        // disable shader program
        glUseProgram(0);

        infologLength = 0;

        if (shaderProgram) glDeleteProgram(shaderProgram);
        if (fragmentShader) glDeleteShader(fragmentShader);

        char *fsc = (char *) malloc(sizeof(char)*(strlen(fragmentShaderHeader)+strlen(fragmentShaderCode)+2));
        strcpy(fsc, fragmentShaderHeader);
        strcat(fsc, "\n");
        strcat(fsc, fragmentShaderCode);
        fsc[strlen(fsc)] = '\0';

        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, (const GLchar **) &fsc, NULL);
        glCompileShader(fragmentShader);
        glGetShaderInfoLog(fragmentShader, 4096, &infologLength, infoLog);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        glGetProgramInfoLog(shaderProgram, 4096, &infologLength, infoLog);

        // use the shader program
        glUseProgram(shaderProgram);

        uniform_texturesize = glGetUniformLocation(shaderProgram, "iChannelResolution[0]");
        glUniform3f(uniform_texturesize, viewport.width, viewport.height, 0.0);
        uniform_viewportsize = glGetUniformLocation(shaderProgram, "iResolution");
        glUniform3f(uniform_viewportsize, viewport.width, viewport.height, 0.0);
        uniform_time = glGetUniformLocation(shaderProgram, "iGlobalTime");
        uniform_channeltime = glGetUniformLocation(shaderProgram, "iChannelTime[0]");
        uniform_date = glGetUniformLocation(shaderProgram, "iDate");

        // do not recompile shader next time
        code_changed = false;
    }

    // use the shader program
    glUseProgram(shaderProgram);

    // set time uniforms
    glUniform1f(uniform_time, m_curTime);
    glUniform1f(uniform_channeltime, m_curTime);
    std::time_t now = std::time(0);
    std::tm *local = std::localtime(&now);
    glUniform4f(uniform_date, local->tm_year, local->tm_mon, local->tm_mday, local->tm_hour*3600.0+local->tm_min*60.0+local->tm_sec);

    // activate the fbo2 as our render target
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObject);

    //render the original texture on a quad in fbo
    drawQuad( viewport, Texture);

    // disable shader program
    glUseProgram(0);

    // (re)activate the HOST fbo as render target
    glBindFramebufferEXT(GL_FRAMEBUFFER, pGL->HostFBO);

    // render the fbo2 texture texture on a quad in the host fbo
    drawQuad( viewport, textureFrameBufferObject );

    //unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    //disable texturemapping
    glDisable(GL_TEXTURE_2D);

    return FF_SUCCESS;
}


#ifdef FF_FAIL  // FFGL 1.5

#ifdef _WIN32

__declspec(dllexport) bool __stdcall setString(unsigned int t, const char *string, DWORD instanceID)

#else

bool setString(unsigned int t, const char *string, DWORD *instanceID)

#endif

#else

bool setString(unsigned int t, const char *string, FFInstanceID *instanceID)

#endif
{
    // declare pPlugObj (pointer to this instance)
    // & typecast instanceid into pointer to a CFreeFrameGLPlugin
    FreeFrameShadertoy* pPlugObj = (FreeFrameShadertoy*) instanceID;

    if (pPlugObj) {
        switch (t) {
        case 0:
            pPlugObj->setFragmentProgramCode(string);
            //        case 3:
            //            PluginInfo.GetPluginExtendedInfo()->About = strdup(string);
        }
        return true;
    }

    return false;
}



#ifdef FF_FAIL  // FFGL 1.5

#ifdef _WIN32

__declspec(dllexport) char * __stdcall getString(unsigned int t, DWORD instanceID)


#else

char *getString(unsigned int t, DWORD instanceID)

#endif

#else

char *getString(unsigned int t, FFInstanceID instanceID)

#endif
{

    // declare pPlugObj (pointer to this instance)
    // & typecast instanceid into pointer to a CFreeFrameGLPlugin
    FreeFrameShadertoy* pPlugObj = (FreeFrameShadertoy*) instanceID;

    char *stringtoreturn;
    const char *codetoread;

    if (pPlugObj) {

        switch (t) {
        case 0:
            codetoread =  pPlugObj->getFragmentProgramCode();
            break;
        case 1:
            codetoread =   pPlugObj->getFragmentProgramLogs();
            break;
        case 2:
            codetoread =   fragmentShaderHeader;
            break;
        default:
        case 3:
            codetoread =   fragmentShaderDefaultCode;
            break;
        }

        stringtoreturn = (char *) malloc(sizeof(char)*(strlen(codetoread)+1));
        strcpy(stringtoreturn, codetoread);
        return stringtoreturn;

    }

    return 0;
}
