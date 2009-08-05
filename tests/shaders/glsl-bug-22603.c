/* The original code is:
 * Author: Edwin Moehlheinrich
 * This piece of code is public domain, just don't get yourself killed.
 *
 * Subsequent modifications to the code is:
 * Copyright © 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Edwin Moehlheinrich
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#if defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include "GL/glut.h"
#endif

#include "piglit-util.h"

static GLboolean Automatic;

#define WIN_WIDTH 800
#define WIN_HEIGHT 600

/* Misbehaviour: first: the quads are not drawn in the correct order
 * (darker equals closer to the viewer), second: the middle one is strangely
 * distorted.
 */

static GLuint vs, fs, prog, shadow_vs, shadow_fs, shadow_prog;
static GLint shadowMap_location;
static GLint eye_projection_location, light_projection_location;

static void
display(void)
{
	GLuint shadow_texture;
	GLuint fbo;
	float matrix[] = {	2.0/2.0, 0.0, 0.0, 0.0,
				0.0, 2.0/2.0, 0.0, 0.0,
				0.0, 0.0, -2/10.0, -12.0/10,
				0.0, 0.0, 0.0, 1.0};
	float rect1_color[3] = {.3, .3, .3};
	float rect2_color_dark[3] = {.01, .01, .01};
	float rect2_color_bottom_rect1[3] = {.09, .09, .09};
	float rect3_color[3] = {.1, .1, .1};
	GLboolean pass = GL_TRUE;

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glClearDepth(1.0f);
    	glDepthFunc(GL_LEQUAL);

	glUniform1iARB(shadowMap_location, 0);
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &shadow_texture);
	glBindTexture(GL_TEXTURE_2D, shadow_texture);
	glTexImage2D(GL_TEXTURE_2D, 0,
		     GL_DEPTH_COMPONENT,
		     512, 512, 0,
		     GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY);

	glGenFramebuffersEXT(1, &fbo);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
				  GL_DEPTH_ATTACHMENT_EXT,
				  GL_TEXTURE_2D,
				  shadow_texture,
				  0);

	glViewport(0, 0, 512, 512);
	glClear(GL_DEPTH_BUFFER_BIT);
	glUseProgramObjectARB(shadow_prog);
	/* ogl reads in col-vectors, so transform=true */
	glUniformMatrix4fvARB(light_projection_location,
			      1, 1, matrix);
	glBegin(GL_QUADS);
		/* rect1 */
		glVertex3f(-0.4,   0.4,   -2.0);
		glVertex3f(-0.4,  -0.4,   -2.0);
		glVertex3f(0.4,  -0.4,   -2.0);
		glVertex3f(0.4,   0.4,   -2.0);

		/* rect2 */
		glVertex3f(-0.2,   0.5,   -7.0);
		glVertex3f(-0.2,  -0.3,   -1.0);
		glVertex3f(0.6,  -0.3,   -1.0);
		glVertex3f(0.6,   0.5,   -7.0);

		/* rect3 */
		glVertex3f(-0.0,   0.6,   -4.0);
		glVertex3f(-0.0,  -0.2,   -4.0);
		glVertex3f(0.8,  -0.2,   -4.0);
		glVertex3f(0.8,   0.6,   -4.0);
	glEnd();

	/* bind back the backbuffer and display the depthmap */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glViewport(0, 0, 600, 600);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_2D, shadow_texture);

	glUseProgramObjectARB(prog);
	glUniformMatrix4fvARB(eye_projection_location, 1, 1, matrix);
	/* this is the quad that is texturized with what we want to see */
	glBegin(GL_QUADS);

		glTexCoord2d(0.0,1.0);
		glVertex3f(-0.9,   0.9,   -1);

		glTexCoord2d(0.0,0.0);
		glVertex3f(-0.9,  -0.9,   -1);

		glTexCoord2d(1.0,0.0);
		glVertex3f(0.9,  -0.9,   -1);

		glTexCoord2d(1.0,1.0);
		glVertex3f(0.9,   0.9,   -1);

	glEnd();

	/* check that rect1 is present. */
	pass &= piglit_probe_pixel_rgb(500, 400, rect1_color);
	/* check that rect3 is present. */
	pass &= piglit_probe_pixel_rgb(220, 300, rect3_color);
	/* check that rect2 where greater than rect1 is not rendered */
	pass &= piglit_probe_pixel_rgb(450, 350, rect1_color);
	/* check that rect2 where greater than rect3 is not rendered */
	pass &= piglit_probe_pixel_rgb(270, 350, rect3_color);
	/* check that rect2 where less than rect3 is rendered */
	pass &= piglit_probe_pixel_rgb(270, 225, rect2_color_dark);
	/* check that rect2 where less than rect1 is rendered */
	pass &= piglit_probe_pixel_rgb(450, 250, rect2_color_bottom_rect1);

	if (Automatic)
		piglit_report_result (pass ? PIGLIT_SUCCESS : PIGLIT_FAILURE);

	glutSwapBuffers();
}

static void
init(void)
{
	const char *vs_source =
		"uniform mat4 eye_projection;"
		"varying vec2 texture_coords;"
		"void main()"
		"{"
		"	gl_Position = eye_projection * gl_Vertex;"
		"	texture_coords = gl_MultiTexCoord0.st;"
		"}";
	const char *fs_source =
		"uniform sampler2D shadowMap;"
		"varying vec2 texture_coords;"
		"void main()"
		"{"
		"	float map_depth = texture2D(shadowMap,texture_coords).a;"
		"	gl_FragColor =  vec4(1.0, 1.0, 1.0, 1.0)* map_depth;"
		"}";
	const char *vs_shadow_source =
		"uniform mat4 light_projection; "
		"void main() "
		"{"
		"	gl_Position = light_projection * gl_Vertex;"
		"}";
	const char *fs_shadow_source =
		"void main()"
		"{"
		"	gl_FragDepth = gl_FragCoord.z;"
		"}";

	vs = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	glShaderSourceARB(vs, 1, &vs_source, NULL);
	glCompileShaderARB(vs);

	fs = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glShaderSourceARB(fs, 1, &fs_source, NULL);
	glCompileShaderARB(fs);
	prog = piglit_link_simple_program(vs, fs);

	shadow_vs = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	glShaderSourceARB(shadow_vs, 1, &vs_shadow_source, NULL);
	glCompileShaderARB(shadow_vs);

	shadow_fs = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glShaderSourceARB(shadow_fs, 1, &fs_shadow_source, NULL);
	glCompileShaderARB(shadow_fs);
	shadow_prog = piglit_link_simple_program(shadow_vs, shadow_fs);

	eye_projection_location = glGetUniformLocationARB(prog,
							  "eye_projection");
	light_projection_location = glGetUniformLocationARB(shadow_prog,
							    "light_projection");
	shadowMap_location = glGetUniformLocationARB(shadow_prog, "shadowMap");
}

int
main(int argc, char** argv)
{
	/* far = 11, near = 1 */
	int i;

	glutInit(&argc, argv);

	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-auto"))
			Automatic = 1;
		else
			printf("Unknown option: %s\n", argv[i]);
	}

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
	glutCreateWindow("glsl-bug-22603");
	glutKeyboardFunc(piglit_escape_exit_key);
	glutDisplayFunc(display);
	glewInit();

	if (!GLEW_VERSION_2_0) {
		printf("Requires OpenGL 2.0\n");
		piglit_report_result(PIGLIT_SKIP);
		exit(1);
	}

	init();

	glutMainLoop();

	return 0;
}
