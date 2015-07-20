/*
 * http://www.cnblogs.com/liangliangh/
 --------------------------------------------------------------------------------*/

#ifndef _GL_STAFF_
#define _GL_STAFF_


#include <GL/glew.h>
#include <GL/glut.h>
#include "GLFW/glfw3.h"
#define GLM_FORCE_RADIANS // need not for glm 9.6
#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <map>
#include <cmath>
#include <cwchar>
#include <cstdio>
#include <ctime>

#include <omp.h>

/*------------------------------------ Usage --------------------------------------
#include "gl_staff.h"
#include <iostream>
void draw_world() { glStaff::xyz_frame(2,2,2,true); }
void draw_model() { glutSolidTeapot(1); glStaff::xyz_frame(1,1,1,false); }
void draw(const glm::mat4& mat_model, const glm::mat4& mat_view)
{
	double t = omp_get_wtime();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW); glLoadMatrixf(&mat_view[0][0]);
		draw_world();
	glMatrixMode(GL_MODELVIEW); glMultMatrixf(&mat_model[0][0]);
		draw_model();
	t = omp_get_wtime()-t;
	char st[50]; sprintf(st, "draw time (ms) :  %.2f", t*1000);
	glStaff::text_upperLeft(st, 1);
}
void key_p() { std::cout << "key p pressed\n"; }
int main(void)
{
	glStaff::init_win(800, 800, "OpenGL", "C:\\Windows\\Fonts\\msyh.ttf");
	glStaff::init_gl(); // have to be called after glStaff::init_win

	glStaff::set_mat_view(
		glm::lookAt( glm::vec3(5,5,5), glm::vec3(1,1,1), glm::vec3(0,1,0) ) );
	glStaff::set_mat_model(
		glm::translate( glm::vec3(1,1,1) ) );
	glStaff::add_key_callback('P', key_p, L"print");

	glStaff::renderLoop(draw);
}
---------------------------------------------------------------------------------*/

// use namespace glStaff to use the funtions
namespace glStaff{


// use namespace Internal to hide implementation of glStaff
namespace Internal{
	class keyFunc{ public: void(*f)(); const wchar_t* s; };
	static glm::mat4	mat_model, mat_view;// transformation matrix
	static glm::mat4	mat_projection;     // projection matrix
	static float		speed_scale = 0.1f;	// interactive speed factor
	static float		frustum_fovy = 45;	// fovy of frustum
	static float		n_clip=1.0f, f_clip=1.0e5f;
	static GLFWwindow*	curr_window;  // current window
	static int			help_display; // should display help content or not
//	static FTFont*		font_ftgl;    // the ftgl font used to draw text, use wchar_t for chinese~
	static int          font_size=16; // font size in pixels
	static bool			fps_display=true; // display fps at lover left or not
	static std::map<int,keyFunc> key_funcs; // the key-function map

	inline bool save_mat_to_file(const char* file, const glm::mat4& mat){
		std::ofstream outfile; outfile.open( file );
		if( outfile ){
			for(int i=0; i<4; ++i){
				for(int j=0; j<4; ++j)
				{	outfile.width(12); outfile << mat[j][i] << ' '; }
				outfile << '\n';
			}
			outfile.close();
			return true;
		}
		return false;
	}
	inline bool load_mat_from_file(const char* file, glm::mat4& mat){
		std::ifstream infile; infile.open( file );
		if( infile ){
			for(int i=0; i<4; ++i)
				for(int j=0; j<4; ++j) infile >> mat[j][i];
			infile.close();
			return true;
		}
		return false;
	}

}


//----------------------------------- get ,  set ----------------------------------
inline const glm::mat4& get_mat_model(){
	return Internal::mat_model;
}
inline void				set_mat_model(const glm::mat4& mat){
	Internal::mat_model = mat;
}
inline void				load_mat_model(const char* file){
	glm::mat4 mat;
	if(Internal::load_mat_from_file(file,mat)){
		set_mat_model(mat);
		std::cout << "matrix_model load from file: " << file << '\n';
	}
}

inline const glm::mat4& get_mat_view(){
	return Internal::mat_view;
}
inline void				set_mat_view(const glm::mat4& mat){
	Internal::mat_view = mat;
}
inline void				load_mat_view(const char* file){
	glm::mat4 mat;
	if(Internal::load_mat_from_file(file,mat)){
		set_mat_view(mat);
		std::cout << "matrix_view load from file: " << file << '\n';
	}
}

inline const glm::mat4& get_mat_projection(){
	return Internal::mat_projection;
}
inline void				set_mat_projection(const glm::mat4& mat){
	Internal::mat_projection = mat;
	glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(&mat[0][0]);
	glMatrixMode(GL_MODELVIEW);
}

inline int  get_frame_width(){
	int width, height;
	glfwGetFramebufferSize(Internal::curr_window, &width, &height);
	return width;
}
inline int  get_frame_height(){
	int width, height;
	glfwGetFramebufferSize(Internal::curr_window, &width, &height);
	return height;
}
inline void get_frame_size(int* width, int* height){
	glfwGetFramebufferSize(Internal::curr_window, width, height);
}

// do not use F1,WSAD,Up,Down,Left,Right,Home,End,PageUp,PageDown
inline void add_key_callback(int key, void(*func)(), const wchar_t* description)
{
	if(func==0) return;
	Internal::keyFunc kf; kf.f=func; kf.s=description;
	Internal::key_funcs[key] = kf;
}


//----------------------------------- callbacks -----------------------------------
namespace Internal {
	inline void callback_error(int error, const char* description)
	{
		std::cout << "GLFW Error code: " << error
			<< "\t\tDescription: " << description << '\n';
		std::cin.get(); // hold the screen
	}
	// at pressent, this is the same as window size callback
	
	inline void callback_frameBufferSize(GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
		glMatrixMode(GL_PROJECTION);
		//glLoadIdentity();
		//gluPerspective(frustum_fovy, float(width)/height, n_clip, f_clip);
		mat_projection = glm::perspective(
			glm::radians(frustum_fovy), float(width)/height, n_clip, f_clip );
		glLoadMatrixf(&mat_projection[0][0]);
		glMatrixMode(GL_MODELVIEW);
	}
	
	/* http://www.cnblogs.com/liangliangh/p/4089582.html */
	inline void trackball(float* theta, glm::vec3* normal,
		float ax, float ay, float bx, float by, float r)
	{
		float r2 = r * 0.9f;
		float da = std::sqrt(ax*ax+ay*ay);
		float db = std::sqrt(bx*bx+by*by);
		if(std::max(da,db)>r2){
			float dx, dy;
			if(da>db){
				dx = (r2/da-1)*ax;
				dy = (r2/da-1)*ay;
			}else{
				dx = (r2/db-1)*bx;
				dy = (r2/db-1)*by;
			}
			ax += dx; ay +=dy; bx += dx; by += dy;
		}
		float az = std::sqrt( r*r-(ax*ax+ay*ay) );
		float bz = std::sqrt( r*r-(bx*bx+by*by) );
		glm::vec3 a = glm::vec3(ax,ay,az);
		glm::vec3 b = glm::vec3(bx,by,bz);
		*theta = std::acos( glm::dot(a,b)/(r*r) );
		*normal = glm::cross( a, b );
	}

	static const char* help_string[]={
		"F1 for help",
		"WSAD (Up,Down,Left,Right) :  walk through",
		"PageUp,PageDown,Home,End :  rise,fall,roll",
		"Mouse right move :  change view angle",
		"Mouse left move (left Ctrl):  trackball for model (rotate the world)",
		"Mouse mid move (left Ctrl,scroll) :  move the model (world,scale)",
//		"F11 (left Ctrl) :  screenshot (depth) as png",
		"F9,F10 (left Ctrl) :  load,save current view (model) matrix from,as file",
		"F2 :  toggle FPS display",
		"F3 :  toggle lighting",
		"F4 :  toggle polygon mode",
	};
	
	// action: press, release, repeat  mods: mod_shift,ctrl,alt
	inline void callback_key(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		//std::cout << key << " " << action << " " << mods << "\n";
		if(action==GLFW_PRESS/*action!=GLFW_RELEASE*/){
			switch(key){
			/*case GLFW_KEY_W:*/ case GLFW_KEY_UP:
				mat_view = glm::translate(glm::vec3(0,0, speed_scale)) * mat_view;
				break;
			/*case GLFW_KEY_S:*/ case GLFW_KEY_DOWN:
				mat_view = glm::translate(glm::vec3(0,0,-speed_scale)) * mat_view;
				break;
			/*case GLFW_KEY_A:*/ case GLFW_KEY_LEFT:
				mat_view = glm::translate(glm::vec3( speed_scale,0,0)) * mat_view;
				break;
			/*case GLFW_KEY_D:*/ case GLFW_KEY_RIGHT:
				mat_view = glm::translate(glm::vec3(-speed_scale,0,0)) * mat_view;
				break;
			case GLFW_KEY_PAGE_UP:{
				glm::vec3 v = glm::vec3(mat_view*glm::vec4(0,1,0,0));
				mat_view = glm::translate(-speed_scale*v) * mat_view; }
				break;
			case GLFW_KEY_PAGE_DOWN:{
				glm::vec3 v = glm::vec3(mat_view*glm::vec4(0,1,0,0));
				mat_view = glm::translate( speed_scale*v) * mat_view; }
				break;
			case GLFW_KEY_HOME:
				mat_view = glm::rotate( speed_scale/5, glm::vec3(0,0,1)) * mat_view;
				break;
			case GLFW_KEY_END:
				mat_view = glm::rotate(-speed_scale/5, glm::vec3(0,0,1)) * mat_view;
				break;
			case GLFW_KEY_F1:
				help_display = (help_display+1)%3;
				break;
			case GLFW_KEY_F2:
				fps_display = !fps_display;
				break;
			case GLFW_KEY_F3:{
				if(glIsEnabled(GL_LIGHTING)) glDisable(GL_LIGHTING);
				else glEnable(GL_LIGHTING); }
				break;
			case GLFW_KEY_F4:{
				GLint modes[2]; glGetIntegerv(GL_POLYGON_MODE, modes);
				if      (modes[0]==GL_FILL){
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				}else if(modes[0]==GL_LINE){
					glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				}else if(modes[0]==GL_POINT){
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				} }
				break;
			case GLFW_KEY_F9:{
				if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)!=GLFW_RELEASE){
					glm::mat4 mat;
					if(load_mat_from_file("matrix_model",mat)){
						mat_model = mat;
						std::cout << "matrix_model load: F9 (Ctrl)" << '\n';
					}
				}else{
					glm::mat4 mat;
					if(load_mat_from_file("matrix_view",mat)){
						mat_view = mat;
						std::cout << "matrix_view load: F9" << '\n';
					}
				} }
				break;
			case GLFW_KEY_F10:{
				if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)!=GLFW_RELEASE){
					if(save_mat_to_file("matrix_model",mat_model)){
						std::cout << "matrix_model save: F10 (Ctrl)" << '\n';
					}
				}else{
					if(save_mat_to_file("matrix_view",mat_view)){
						std::cout << "matrix_view save: F10" << '\n';
					}
				} }
				break;
			case GLFW_KEY_F11: //{
//				time_t rawtime; wchar_t buffer[50]; double t; std::wstring ws;
//				std::time(&rawtime); t=omp_get_wtime(); // get time
//				std::wcsftime( buffer, sizeof(buffer)/sizeof(wchar_t),
//					L"%Y-%m-%d %H.%M.%S", std::localtime(&rawtime) );
//				ws += buffer;
//				swprintf(buffer, L"%.3lf", t-std::floor(t));
//				ws += &buffer[1];
//				ws += L".png";
//				int w, h; get_frame_size(&w, &h);
//				if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)!=GLFW_RELEASE){
//					il_saveImgWinDep(ws.c_str(), 0, 0, w, h);
//					std::wcout << "Screenshot (depth) :  " << ws << '\n';
//				}else{
//					il_saveImgWin(ws.c_str(), 0, 0, w, h);
//					std::wcout << "Screenshot (color) :  " << ws << '\n';
//				} }
				break;
			default:
				if(key_funcs.find(key)!=key_funcs.end()){
					if(key_funcs[key].f) (*key_funcs[key].f)();
				}
			}
		}
	}
	
	// button: left, right, mid  action: press, release
	inline void callback_mousePress(GLFWwindow* window, int button, int action, int mods)
	{
		if(action==GLFW_PRESS && button==GLFW_MOUSE_BUTTON_RIGHT){
			//glfwSetCursorPos(window, get_frame_width()/2.0, get_frame_height()/2.0);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		if(action==GLFW_RELEASE && button==GLFW_MOUSE_BUTTON_RIGHT){
			//glfwSetCursorPos(window, get_frame_width()/2.0, get_frame_height()/2.0);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
	
	inline void callback_scroll(GLFWwindow* window, double xoffset, double yoffset)
	{
		mat_view = glm::translate(glm::vec3(0,0, -speed_scale*5*float(yoffset))) * mat_view;
	}
	
	// xpos,ypos: the new xy-coordinate, in screen coordinates, of the cursor
	inline void callback_mouseMove(GLFWwindow* window, double xpos, double ypos)
	{
		static double xpos_last, ypos_last;
		ypos = get_frame_height()-ypos; // window use upper left as origin, but gl use lower left
		if(glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS)
		{
			float dx = float(xpos-xpos_last), dy = float(ypos-ypos_last);
			if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)!=GLFW_RELEASE){ // key left Ctrl is pressed
				mat_view *= glm::rotate(speed_scale/50*dx, glm::vec3(0,1,0));
				glm::vec3 v = glm::vec3(glm::affineInverse(mat_view)*glm::vec4(1,0,0, 0));
				mat_view *= glm::rotate(-speed_scale/50*dy, v);
			}else{
				float theta; glm::vec3 n;
				int width, height; glfwGetFramebufferSize(window, &width, &height);
				trackball( &theta, &n,
					float(xpos_last)-width/2.0f, float(ypos_last)-height/2.0f,
					float(xpos)-width/2.0f, float(ypos)-height/2.0f, std::min(width,height)/4.0f );
				glm::vec3 normal = glm::vec3(
					glm::affineInverse(mat_model) * glm::affineInverse(mat_view)
					* glm::vec4(n.x, n.y, n.z, 0) );
				mat_model *= glm::rotate(theta, normal);
			}
		}
		if(glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS)
		{
			float dx = float(xpos-xpos_last), dy = float(ypos-ypos_last);
			if( dy!=0 )
				mat_view = glm::rotate(-speed_scale/50*dy, glm::vec3(1,0,0) ) * mat_view;
			if( dx!=0 )
				mat_view = glm::rotate( speed_scale/50*dx,	glm::vec3( mat_view * glm::vec4(0,1,0,0) ) ) * mat_view;
		}
		if(glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_MIDDLE)==GLFW_PRESS)
		{
			float dx = float(xpos-xpos_last), dy = float(ypos-ypos_last);
			if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)!=GLFW_RELEASE){ // key left Ctrl is pressed
				mat_view = glm::translate( glm::vec3(speed_scale/5*dx, speed_scale/5*dy, 0) ) * mat_view;
			}else{
				glm::vec4 v = glm::affineInverse(mat_view) * glm::vec4(dx,dy,0,0);
				mat_model = glm::translate( glm::vec3(speed_scale/5*v) ) * mat_model;
			}
		}
		xpos_last = xpos; ypos_last = ypos;
	}

}


//----------------------------------- utilities -----------------------------------
/* hue:0-360; saturation:0-1; lightness:0-1
 * hue:        red(0) -> green(120) -> blue(240) -> red(360)
 * saturation: gray(0) -> perfect colorful(1)
 * lightness:  black(0) -> perfect colorful(0.5) -> white(1)
 * http://blog.csdn.net/idfaya/article/details/6770414
*/
inline void hsl_to_rgb( float h, float s, float l, float* rgb )
{
	if( s==0 ) { rgb[0] = rgb[1] = rgb[2] = l; return; }
	float q, p, hk, t[3];
	if( l<0.5f ) { q = l * ( 1 + s ); }
	else		 { q = l + s - l * s; }
	p = 2 * l - q;
	hk = h / 360;
	t[0] = hk + 1/3.0f;
	t[1] = hk;
	t[2] = hk - 1/3.0f;
	for( int i=0; i<3; ++i ) {
		if     ( t[i]<0 ) { t[i] += 1; }
		else if( t[i]>1 ) { t[i] -= 1; }
	}
	for( int i=0; i<3; ++i ) {
		if     ( t[i] < 1/6.0f ){ rgb[i] = p + (q-p)*6*t[i]; }
		else if( t[i] < 1/2.0f ){ rgb[i] = q; }
		else if( t[i] < 2/3.0f ){ rgb[i] = p + (q-p)*6*(2/3.0f-t[i]); }
		else					{ rgb[i] = p; }
	}
}

inline float rgb_to_gray( float r, float g, float b )
{
	return r*0.299f + g*0.587f + b*0.114f;
}

inline void xyz_frame(float xlen, float ylen, float zlen, bool solid)
{
	if(solid){ // yz frame as solid arrows
		GLfloat color_gls[4]; glGetMaterialfv(GL_FRONT, GL_SPECULAR, color_gls);
		GLfloat color_gla[4]; glGetMaterialfv(GL_FRONT, GL_AMBIENT, color_gla);
		GLfloat color_gld[4]; glGetMaterialfv(GL_FRONT, GL_DIFFUSE, color_gld);

		GLfloat color[]={0, 0, 0, 1};
		glMaterialfv(GL_FRONT, GL_SPECULAR, color);

		glMatrixMode(GL_MODELVIEW);
		color[0]=1; color[1]=0.5f; color[2]=0; // o
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
		glPushMatrix();
			glutSolidSphere(std::max(std::max(xlen,ylen),zlen)/40, 50, 50);
		glPopMatrix();
		color[0]=1; color[1]=0; color[2]=0; // x
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
		glPushMatrix(); glTranslatef(xlen/2, 0, 0); glScalef(1.0f, 0.01f, 0.01f);
			glutSolidCube(xlen);
		glPopMatrix();
		glPushMatrix(); glTranslatef(xlen, 0, 0); glRotatef(90, 0, 1, 0);
			glutSolidCone(xlen/30, xlen/8, 50, 50);
		glPopMatrix();
		color[0]=0; color[1]=1; color[2]=0; // y
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
		glPushMatrix(); glTranslatef(0, ylen/2, 0); glScalef(0.01f, 1.0f, 0.01f);
			glutSolidCube(ylen);
		glPopMatrix();
		glPushMatrix(); glTranslatef(0, ylen, 0); glRotatef(-90, 1, 0, 0);
			glutSolidCone(ylen/30, ylen/8, 50, 50);
		glPopMatrix();
		color[0]=0; color[1]=0; color[2]=1; // z
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
		glPushMatrix(); glTranslatef(0, 0, zlen/2); glScalef(0.01f, 0.01f, 1.0f);
			glutSolidCube(zlen);
		glPopMatrix();
		glPushMatrix(); glTranslatef(0, 0, zlen);
			glutSolidCone(zlen/30, zlen/8, 50, 50);
		glPopMatrix();
		color[0]=1; color[1]=0; color[2]=1; // unit
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
		glPushMatrix(); glTranslatef(1,0,0); glutSolidCube(xlen/50); glPopMatrix();
		glPushMatrix(); glTranslatef(0,1,0); glutSolidCube(ylen/50); glPopMatrix();
		glPushMatrix(); glTranslatef(0,0,1); glutSolidCube(zlen/50); glPopMatrix();

		glMaterialfv(GL_FRONT, GL_SPECULAR, color_gls);
		glMaterialfv(GL_FRONT, GL_AMBIENT, color_gla);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, color_gld);
	}else{ // xyz frame as lines
		GLfloat psize; glGetFloatv(GL_POINT_SIZE, &psize);
		GLfloat lsize; glGetFloatv(GL_LINE_WIDTH, &lsize);
		GLfloat cgl[4]; glGetFloatv(GL_CURRENT_COLOR, cgl);
		GLboolean light = glIsEnabled(GL_LIGHTING);
		if(GL_TRUE==light) glDisable(GL_LIGHTING);
		glPointSize(8); glLineWidth(2);
		GLfloat color[]={0, 0, 0, 1};
		glBegin(GL_LINES);
			color[0]=1; color[1]=0; color[2]=0; // x
			glColor3fv(color);
			glVertex3f(0, 0, 0);
			glVertex3f(xlen, 0, 0);
			color[0]=0; color[1]=1; color[2]=0; // y
			glColor3fv(color);
			glVertex3f(0, 0, 0);
			glVertex3f(0, ylen, 0);
			color[0]=0; color[1]=0; color[2]=1; // z
			glColor3fv(color);
			glVertex3f(0, 0, 0);
			glVertex3f(0, 0, zlen);
		glEnd();
		glBegin(GL_POINTS);
			color[0]=1; color[1]=0.5f; color[2]=0; // o
			glColor3fv(color);
			glVertex3f(0,0,0);
		glEnd();
		glPointSize(psize); glLineWidth(lsize);
		glColor4fv(cgl);
		if(GL_TRUE==light) glEnable(GL_LIGHTING);
	}
}

namespace Internal{
	static int line_last=0;
}

inline void text_upperLeft(const wchar_t* ws, int line=0)
{
	if(line<=0) line=Internal::line_last+1;

//	int ox = Internal::font_size, oy = get_frame_height()-Internal::font_size*2; // upper left of the window
//	float linespace = Internal::font_size*1.3f; // line spacing
//	float l = Internal::font_ftgl->Advance(ws);
	GLint dm; glGetIntegerv(GL_DEPTH_WRITEMASK, &dm);
	glDepthMask(GL_FALSE);
//		Internal::font_ftgl->Render(ws, -1, FTPoint(ox,oy-linespace*(line-1)));
	glDepthMask(dm);

	Internal::line_last=line;
}
inline void text_upperLeft(const char* s, int line=0)
{
	if(line<=0) line=Internal::line_last+1;

//	int ox = Internal::font_size, oy = get_frame_height()-Internal::font_size*2; // upper left of the window
//	float linespace = Internal::font_size*1.3f; // line spacing
//	float l = Internal::font_ftgl->Advance(s);
	GLint dm; glGetIntegerv(GL_DEPTH_WRITEMASK, &dm);
	glDepthMask(GL_FALSE);
//		Internal::font_ftgl->Render(s, -1, FTPoint(ox,oy-linespace*(line-1)));
	glDepthMask(dm);

	Internal::line_last=line;
}


//----------------------------------- initialize ----------------------------------
inline void init_win(int width, int height, const char* tile,
	const char* font_file )
{
	// glfw init
	glfwSetErrorCallback(Internal::callback_error);
	if( !glfwInit() ){
		std::cout << "GLFW init Error";
		std::cin.get(); // hold the screen
	}

	// create window
	glfwWindowHint(GLFW_SAMPLES, 8); // anti-aliase, the RGBA,depth,stencil are set by default
	const GLFWvidmode* mods = glfwGetVideoMode(glfwGetPrimaryMonitor());
	Internal::curr_window = glfwCreateWindow(width, height, tile, 0, 0);
	if(!Internal::curr_window){
		std::cout << "Create window Error";
		std::cin.get(); // hold the screen
	}
	// window at center of screem
	glfwSetWindowPos(Internal::curr_window,
		std::max(4,mods->width/2-width/2), std::max(24,mods->height/2-height/2));
	glfwMakeContextCurrent(Internal::curr_window);
	glfwSetFramebufferSizeCallback(
		Internal::curr_window, Internal::callback_frameBufferSize );
	glfwSetKeyCallback(
		Internal::curr_window, Internal::callback_key );
	glfwSetCursorPosCallback(
		Internal::curr_window, Internal::callback_mouseMove );
	glfwSetScrollCallback(
		Internal::curr_window, Internal::callback_scroll );
	glfwSetMouseButtonCallback(
		Internal::curr_window, Internal::callback_mousePress );

	// glut init
	int argc=0;
	glutInit( &argc, NULL );

	// glew init, have to be after the GL context has been created
	GLenum err = glewInit();
	if(GLEW_OK != err) {
		std::cout << "GLEW init Error: " << glewGetErrorString(err);
		std::cin.get(); // hold the screen
	}
	
	// image library init
//	il_init();

	// font loading
	//Internal::font_ftgl = new FTBitmapFont(font_file); // far more fast
//	Internal::font_ftgl = new FTPixmapFont(font_file);
//	if(Internal::font_ftgl->Error()){
//		std::cout << "Front load Error";
//		std::cin.get();
//	}
//	Internal::font_ftgl->FaceSize(Internal::font_size);
//	Internal::font_ftgl->Depth(Internal::font_size/2.0f);
//	Internal::font_ftgl->UseDisplayList(false);

}

inline void init_gl()
{
	// projection matrix
	glMatrixMode(GL_PROJECTION);
	int w, h; get_frame_size(&w, &h);
	//glLoadIdentity();
	//gluPerspective(Internal::frustum_fovy, float(w)/h,
	//	Internal::n_clip, Internal::f_clip);
	Internal::mat_projection = glm::perspective(glm::radians(Internal::frustum_fovy),
		float(w)/h, Internal::n_clip, Internal::f_clip );
	glLoadMatrixf(&Internal::mat_projection[0][0]);
	// model-view matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// color
	glClearColor(0, 0, 0.25f, 1);
	glColor4f(.5f, .5f, .5f, 1);
	glShadeModel(GL_SMOOTH);

	// material
	GLfloat c[]={.7f, .7f, .7f, 1};
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, c); // front, gray
	c[0]=.4f; c[1]=.4f; c[2]=.4f;
	glMaterialfv(GL_FRONT, GL_SPECULAR, c);
	glMaterialf(GL_FRONT, GL_SHININESS, 50);
	c[0]=0; c[1]=0; c[2]=0;
	glMaterialfv(GL_BACK, GL_AMBIENT_AND_DIFFUSE, c); // back, black
//	glMaterialfv(GL_BACK, GL_SPECULAR, c);

	// lighting, light0
	GLfloat vec4f[]={1, 1, 1, 1};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, vec4f); // white DIFFUSE, SPECULAR
	glLightfv(GL_LIGHT0, GL_SPECULAR, vec4f);
	vec4f[0]=.0f; vec4f[1]=.0f; vec4f[2]=.0f; 
	glLightfv(GL_LIGHT0, GL_AMBIENT, vec4f); // black AMBIENT
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE); // LOCAL_VIEWER
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE); // single side
	vec4f[0]=0.25f; vec4f[1]=0.25f; vec4f[2]=0.25f;
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, vec4f); // global AMBIENT lighting, gray

	//glEnable(GL_CULL_FACE); glCullFace(GL_BACK); glFrontFace(GL_CCW);
	glDisable(GL_CULL_FACE);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);

	// blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_NORMALIZE);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
}

namespace Internal{
	inline void FPS_lowerLeft(double period=0.5)
	{
		static double time_last=omp_get_wtime();
		static int number=0;
		static double fps=0;

		++ number;
		double time_now = omp_get_wtime();
		double time_s = time_now-time_last;
		if(time_s >= period){
			fps = number/time_s;
			time_last=time_now; number=0;
		}
		char t[50]; sprintf(t, "FPS :  %.2lf", fps);
		GLint dm; glGetIntegerv(GL_DEPTH_WRITEMASK, &dm);
		glDepthMask(GL_FALSE);
//			font_ftgl->Render(t, -1, FTPoint(font_size,font_size));
		glDepthMask(dm);
	}
	
	inline void helpDisplay_upperRight()
	{
		GLint dm; glGetIntegerv(GL_DEPTH_WRITEMASK, &dm);
		glDepthMask(GL_FALSE);
		int ox, oy;
		glfwGetFramebufferSize(curr_window, &ox, &oy);
		ox -= font_size; oy -= font_size*2; // upper right of the window
//		float linespace = font_size*1.3f; // line spacing
		if(help_display==0){
//			float l = font_ftgl->Advance("F1 for help");
//			font_ftgl->Render("F1 for help", -1, FTPoint(ox-l,oy));
		}
		if(help_display==1){
//			float l; int i;
//			for(i=0; i<sizeof(help_string)/sizeof(char*); ++i){
//				l = font_ftgl->Advance(help_string[i]);
//				font_ftgl->Render(
//					help_string[i], -1, FTPoint(ox-l,oy-linespace*i));
//			}
//			for(std::map<int,keyFunc>::iterator it=
//				key_funcs.begin(); it!=key_funcs.end(); ++it){
//					std::wstring ws = std::wstring(L"Key ")
//						+ wchar_t(it->first) + L" :  " + it->second.s;
//					l = font_ftgl->Advance(ws.c_str());
//					font_ftgl->Render(
//						ws.c_str(), -1, FTPoint(ox-l,oy-linespace*(++i)) );
//			}
		}
		glDepthMask(dm);
	}

}

inline void renderLoop(void(*draw)(const glm::mat4&, const glm::mat4&))
{
	static double t1, t2, t3;
#define TIME_START(n) t##n=omp_get_wtime()
#define TIME_END(n)   t##n=omp_get_wtime()-t##n
#define TIME_TEXT(n)  {char st[50]; \
	sprintf(st, "time"#n" (ms) :  %.2lf", t##n*1000); text_upperLeft(st, 3+n);}

	while(!glfwWindowShouldClose(Internal::curr_window))
	{
		TIME_START(1);
		// user's draw function
		draw(Internal::mat_model, Internal::mat_view);

		// draw text of description of User Interface
		Internal::helpDisplay_upperRight();

		// FPS display
		if(Internal::fps_display) Internal::FPS_lowerLeft(0.5);
		TIME_END(1);

		//TIME_TEXT(1); TIME_TEXT(2); TIME_TEXT(3);

		TIME_START(2);
		// swap buffers and poll events
		glfwSwapBuffers(Internal::curr_window);
		TIME_END(2);

		TIME_START(3);
		glfwPollEvents();
		TIME_END(3);
	}
	// no more events will be delivered for that window and its handle becomes invalid
	glfwDestroyWindow(Internal::curr_window);
	// destroys all remaining windows, frees any allocated resources and into an uninitialized
	glfwTerminate();
//	delete Internal::font_ftgl;
}


}


#endif // #ifndef _GL_STAFF_

