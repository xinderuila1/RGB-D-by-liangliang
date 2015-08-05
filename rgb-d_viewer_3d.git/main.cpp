
#include "gl_staff.h"
#include "read_file.h"
#include <iostream>
#include "opencv2/opencv.hpp"

cv::Mat img_rgb, img_depth, img_3d;
std::vector<int> tri_idxes;

//start Add by gaoyu 2015-7-27
cv::Mat img_3d_center;
std::vector<int> tri_idxes_center;
//end



float resolution_X=640, resolution_Y=480, xzFactor=1.12213 * 1.2, yzFactor=0.84160 * 1.2;

bool play_on = false;
bool play_mode = false;
bool translate_center = false;

#define  TO_GLM_VEC3(a)  (*((glm::vec3*)(&(a).x)))

void coor_img2cam(const cv::Mat& img_d, cv::Mat& img_out)
{
	img_out.create(img_d.rows, img_d.cols, CV_32FC3);
//#pragma omp paralell for
	for(int y=0; y<img_d.rows; ++y)
		for(int x=0; x<img_d.cols; ++x){
			cv::Point3f& pc = img_out.at<cv::Point3f>(y,x);
			float d = img_d.at<float>(y,x);
			float xfac = xzFactor * ( x / resolution_X-0.5f );
			float yfac = - yzFactor * ( y / resolution_Y-0.5f );
//			d = std::sqrt(1-xfac*xfac-yfac*yfac) * d;
			pc.x = xfac * d;
			pc.y = yfac * d;
			pc.z = d;
			//pc = pc * (1/1000.0f);
		}

}

void coor_img2cam_by_gaoyu(const cv::Mat& img_d, cv::Mat& img_out)
{
	img_out.create(img_d.rows, img_d.cols, CV_32FC3);

	float center_x = 0;
	float center_y = 0;
	float center_z = 0;


	for(int y=0; y<img_d.rows; ++y)
		for(int x=0; x<img_d.cols; ++x){
			cv::Point3f& pc = img_out.at<cv::Point3f>(y,x);
			float d = img_d.at<float>(y,x);

			//speed-dream中没有绘制的区域,深度置接近1,重建时抛弃这些点 Add by gaoyu 2015-8-5
			if(d > 0.9998)
			{
				pc.x = 0;
				pc.y = 0;
				pc.z = 0;
				continue;
			}


			GLint viewport[4];
			GLdouble modelview[16];
			GLdouble projection[16];
			GLfloat winX, winY, winZ;
			GLdouble posX, posY, posZ;
			glPushMatrix();
			//缩放、平移、旋转等变换 为什么？？？？？？？？？？
//			glScalef(......);
//			glRotatef(......);
//			glTranslatef(......);

			//变换要绘图函数里的顺序一样，否则坐标转换会产生错误
			glGetIntegerv(GL_VIEWPORT, viewport); // 得到的是最后一个设置视口的参数
			glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
			glGetDoublev(GL_PROJECTION_MATRIX, projection);
			glPopMatrix();
			winX = x;
			winY = y;
			winZ = d;//注意记住这个参数,否则有些远的地方纹理贴不上
//			glReadPixels((int)winX, (int)winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
			gluUnProject(winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
			pc.x = posX;
			pc.y = posY;
			pc.z = posZ;


			if(y == 200 && x == 512)
			{
				center_x = pc.x;
				center_y = pc.y + 2;
				center_z = pc.z;
			}
		}


	for(int y=0; y<img_d.rows; ++y)
		for(int x=0; x<img_d.cols; ++x){
			cv::Point3f& pc = img_out.at<cv::Point3f>(y,x);
			float d = img_d.at<float>(y,x);

			pc.x = pc.x - center_x;
			pc.y = pc.y - center_y;
			pc.z = pc.z - center_z;
		}

	//不在找包围盒
}

void coor_img2cam_translate_center(const cv::Mat& img_d, cv::Mat& img_out)
{
	img_out.create(img_d.rows, img_d.cols, CV_32FC3);


	//为了确定,点云的长方体包围盒   Add by gaoyu 2015-7-27
	float max_x = -1.0e+038;
	float max_y = -1.0e+038;
	float max_z = -1.0e+038;
	float min_x = 1.0e+038;
	float min_y = 1.0e+038;
	float min_z = 1.0e+038;


	for(int y=0; y<img_d.rows; ++y)
		for(int x=0; x<img_d.cols; ++x){
			cv::Point3f& pc = img_out.at<cv::Point3f>(y,x);
			float d = img_d.at<float>(y,x);


			GLint viewport[4];
			GLdouble modelview[16];
			GLdouble projection[16];
			GLfloat winX, winY, winZ;
			GLdouble posX, posY, posZ;
			glPushMatrix();
			//缩放、平移、旋转等变换 为什么？？？？？？？？？？
//			glScalef(......);
//			glRotatef(......);
//			glTranslatef(......);

			//变换要绘图函数里的顺序一样，否则坐标转换会产生错误
			glGetIntegerv(GL_VIEWPORT, viewport); // 得到的是最后一个设置视口的参数
			glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
			glGetDoublev(GL_PROJECTION_MATRIX, projection);
			glPopMatrix();
			winX = x;
			winY = y;
			winZ = d;//注意记住这个参数,否则有些远的地方纹理贴不上
//			glReadPixels((int)winX, (int)winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
			gluUnProject(winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
			pc.x = posX;
			pc.y = posY;
			pc.z = posZ;
			//找见长方体对角线最大值点
			if(pc.x > max_x)
			{
				max_x = pc.x;
			}

			if(pc.y > max_y)
			{
				max_y = pc.y;
			}

			if(pc.z > max_z)
			{
				max_z = pc.z;
			}

			//找见长方体对角线最小值点
			if(pc.x < min_x)
			{
				min_x = pc.x;
			}

			if(pc.y < min_y)
			{
				min_y = pc.y;
			}

			if(pc.z < min_z)
			{
				min_z = pc.z;
			}
		}

	for(int y=0; y<img_d.rows; ++y)
		for(int x=0; x<img_d.cols; ++x){
			cv::Point3f& pc = img_out.at<cv::Point3f>(y,x);
			float d = img_d.at<float>(y,x);

			//pc.x = pc.x - (max_x + min_x)*0.5;
			//pc.y = pc.y - (max_y + min_y)*0.5;
			pc.z = pc.z - (max_z + min_z)*0.5;
		}
}

bool nice_tri(const cv::Point3f& p1, const cv::Point3f& p2, const cv::Point3f& p3, float threshhold)
{
	if( glm::length(TO_GLM_VEC3(p1)-TO_GLM_VEC3(p2))>threshhold )
		return false;
	if( glm::length(TO_GLM_VEC3(p2)-TO_GLM_VEC3(p3))>threshhold )
		return false;
	if( glm::length(TO_GLM_VEC3(p3)-TO_GLM_VEC3(p1))>threshhold )
		return false;
	return true;
}

void reconstruct(const cv::Mat& p3d, std::vector<int>& tris)
{
	tris.clear();
	float threashhold = 0.7f;
	for(int i=0; i<p3d.rows-1; ++i)
		for(int j=0; j<p3d.cols-1; ++j){
			#define TRIS_PUSH(i,j)  tris.push_back( (i)*p3d.cols+(j) )
			if( (i+j)%2==0 ){
				if( nice_tri(p3d.at<cv::Point3f>(i,j), p3d.at<cv::Point3f>(i,j+1), p3d.at<cv::Point3f>(i+1,j+1), threashhold) ){
					TRIS_PUSH(i, j);
					TRIS_PUSH(i, j+1);
					TRIS_PUSH(i+1, j+1);
				}
				if( nice_tri(p3d.at<cv::Point3f>(i,j), p3d.at<cv::Point3f>(i+1,j+1), p3d.at<cv::Point3f>(i+1,j), threashhold) ){
					TRIS_PUSH(i, j);
					TRIS_PUSH(i+1, j+1);
					TRIS_PUSH(i+1, j);
				}
			}else{
				if( nice_tri(p3d.at<cv::Point3f>(i,j), p3d.at<cv::Point3f>(i,j+1), p3d.at<cv::Point3f>(i+1,j), threashhold) ){
					TRIS_PUSH(i, j);
					TRIS_PUSH(i, j+1);
					TRIS_PUSH(i+1, j);
				}
				if( nice_tri(p3d.at<cv::Point3f>(i+1,j), p3d.at<cv::Point3f>(i,j+1), p3d.at<cv::Point3f>(i+1,j+1), threashhold) ){
					TRIS_PUSH(i+1, j);
					TRIS_PUSH(i, j+1);
					TRIS_PUSH(i+1, j+1);
				}
			}
		}
}

void draw_world()
{
	glStaff::xyz_frame(2,2,2,false);
}

#define DATA_FILE_PATH "../data/"



//读取数据的部分写在绘制函数里面  Add by gaoyu 2015-7-20
void draw_model_by_gaoyu()
{
//	glutSolidTeapot(1);
//	glStaff::xyz_frame(1,1,1,false);
static int file_i=0, first=1;
	if( play_on || first ){
		first = 0;
		file_i = (file_i+1) % 800;
		char ss[100]; sprintf(ss, "%d", file_i);
		std::string file_name;
		file_name = std::string(DATA_FILE_PATH"rgb/") + ss + ".png";
		read_rgb(file_name.c_str(), img_rgb);
		file_name = std::string(DATA_FILE_PATH"depth/") + ss + ".dat";
		read_depth_by_gaoyu(file_name.c_str(), img_depth);

		coor_img2cam_by_gaoyu(img_depth, img_3d);
		reconstruct(img_3d, tri_idxes);

		//Add by gaoyu 2015-7-27
		//read_depth_by_gaoyu(file_name.c_str(), img_depth_center);
		coor_img2cam_translate_center(img_depth, img_3d_center);
		reconstruct(img_3d, tri_idxes_center);



		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_rgb.cols, img_rgb.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, img_rgb.data);

		std::cout << "img num: " << file_i << "\n";
	}

	if(play_mode){
		if(translate_center)
		{
			GLboolean elt = glIsEnabled(GL_LIGHTING);
			GLboolean tex = glIsEnabled(GL_TEXTURE_2D);
//			glDisable(GL_LIGHTING);
//			glDisable(GL_TEXTURE_2D);

			float tx = 1.0f / img_3d_center.cols, ty = 1.0f / img_3d_center.rows;//顶点纹理映射  Add by gaoyu 2015-7-31

			float rgb[4]={0,1,0, 1};
			glBegin(GL_POINTS);
			for(int i=0; i<img_3d_center.rows; ++i)
				for(int j=0; j<img_3d_center.cols; ++j){
					//不进行hsl到rgb的转换,仅仅用绿色绘制点云  Add by gaoyu 2015-7-27
	//				glStaff::hsl_to_rgb((img_3d.at<cv::Point3f>(i,j).y-1)/4*360,
	//						1, 0.5f, rgb);
					//glColor3fv(rgb);

					glTexCoord2f(j*tx,
							                 i*ty);//顶点纹理映射  Add by gaoyu 2015-7-31

					glVertex3fv(&img_3d_center.at<cv::Point3f>(i,j).x);
				}
			glEnd();

			if(elt) glEnable(GL_LIGHTING);
			if(tex) glEnable(GL_TEXTURE_2D);

			//不仅仅将点云移动到视角中心,还循环移动视角,以对点云做一个概览  Add by gaoyu 2015-7-29
			glStaff::Internal::mat_view *= glm::rotate(float(3.1415926/180.0), glm::vec3(0,1,0));
		}
		else
		{
			GLboolean elt = glIsEnabled(GL_LIGHTING);
			GLboolean tex = glIsEnabled(GL_TEXTURE_2D);
			glDisable(GL_LIGHTING);
			glDisable(GL_TEXTURE_2D);

			float rgb[4]={0,1,0, 1};
			glBegin(GL_POINTS);
			for(int i=0; i<img_3d.rows; ++i)
				for(int j=0; j<img_3d.cols; ++j){
					//不进行hsl到rgb的转换,仅仅用绿色绘制点云  Add by gaoyu 2015-7-27
	//				glStaff::hsl_to_rgb((img_3d.at<cv::Point3f>(i,j).y-1)/4*360,
	//						1, 0.5f, rgb);

					//进行hsl的转换    Add by gaoyu 2015-8-4
					glStaff::hsl_to_rgb((img_3d.at<cv::Point3f>(i,j).y-1)/4*50,
							1, 0.5f, rgb);

					glColor3fv(rgb);
					glVertex3fv(&img_3d.at<cv::Point3f>(i,j).x);
				}
			glEnd();

			if(elt) glEnable(GL_LIGHTING);
			if(tex) glEnable(GL_TEXTURE_2D);
		}
	}
	else if(translate_center){
		GLfloat c[]={.0f, .0f, .0f, 1};
		glMaterialfv(GL_FRONT, GL_SPECULAR, c);
		c[0]=0.7f; c[1]=0.7f; c[2]=0.7f;
		glMaterialfv(GL_FRONT, GL_DIFFUSE, c);
		c[0]=2.0f; c[1]=2.0f; c[2]=2.0f;
		glMaterialfv(GL_FRONT, GL_AMBIENT, c);
		glBegin(GL_TRIANGLES);
		#define  POINT_2D(a)  (img_3d_center.at<cv::Point3f>((a)/img_3d_center.cols,(a)%img_3d_center.cols))
		float tx = 1.0f / img_3d_center.cols, ty = 1.0f / img_3d_center.rows;
		for(unsigned i=0; i<tri_idxes_center.size()/3; ++i){
			glm::vec3 n = glm::normalize(glm::cross(
					TO_GLM_VEC3(POINT_2D(tri_idxes_center[i*3+1]))-TO_GLM_VEC3(POINT_2D(tri_idxes_center[i*3])),
					TO_GLM_VEC3(POINT_2D(tri_idxes_center[i*3+2]))-TO_GLM_VEC3(POINT_2D(tri_idxes_center[i*3]))  ) );
			glNormal3fv(&n[0]);
			glTexCoord2f(tri_idxes_center[i*3]%img_3d_center.cols*tx,tri_idxes_center[i*3]/img_3d_center.cols*ty);
			glVertex3fv(&POINT_2D(tri_idxes_center[i*3]).x);
			glTexCoord2f(tri_idxes_center[i*3+1]%img_3d_center.cols*tx,tri_idxes_center[i*3+1]/img_3d_center.cols*ty);
			glVertex3fv(&POINT_2D(tri_idxes_center[i*3+1]).x);
			glTexCoord2f(tri_idxes_center[i*3+2]%img_3d_center.cols*tx,tri_idxes_center[i*3+2]/img_3d_center.cols*ty);
			glVertex3fv(&POINT_2D(tri_idxes_center[i*3+2]).x);
		}
		glEnd();

		//不仅仅将点云移动到视角中心,还循环移动视角,以对点云做一个概览  Add by gaoyu 2015-7-29
		glStaff::Internal::mat_view *= glm::rotate(float(3.1415926/180.0), glm::vec3(0,1,0));
	}
	else{
		GLfloat c[]={.0f, .0f, .0f, 1};
		glMaterialfv(GL_FRONT, GL_SPECULAR, c);
		c[0]=0.7f; c[1]=0.7f; c[2]=0.7f;
		glMaterialfv(GL_FRONT, GL_DIFFUSE, c);
		c[0]=2.0f; c[1]=2.0f; c[2]=2.0f;
		glMaterialfv(GL_FRONT, GL_AMBIENT, c);
		glBegin(GL_TRIANGLES);
		#define  POINT_1D(a)  (img_3d.at<cv::Point3f>((a)/img_3d.cols,(a)%img_3d.cols))
		float tx = 1.0f / img_3d.cols, ty = 1.0f / img_3d.rows;
		for(unsigned i=0; i<tri_idxes.size()/3; ++i){
			glm::vec3 n = glm::normalize(glm::cross(
					TO_GLM_VEC3(POINT_1D(tri_idxes[i*3+1]))-TO_GLM_VEC3(POINT_1D(tri_idxes[i*3])),
					TO_GLM_VEC3(POINT_1D(tri_idxes[i*3+2]))-TO_GLM_VEC3(POINT_1D(tri_idxes[i*3]))  ) );
			glNormal3fv(&n[0]);
			glTexCoord2f(tri_idxes[i*3]%img_3d.cols*tx,tri_idxes[i*3]/img_3d.cols*ty);
			glVertex3fv(&POINT_1D(tri_idxes[i*3]).x);
			glTexCoord2f(tri_idxes[i*3+1]%img_3d.cols*tx,tri_idxes[i*3+1]/img_3d.cols*ty);
			glVertex3fv(&POINT_1D(tri_idxes[i*3+1]).x);
			glTexCoord2f(tri_idxes[i*3+2]%img_3d.cols*tx,tri_idxes[i*3+2]/img_3d.cols*ty);
			glVertex3fv(&POINT_1D(tri_idxes[i*3+2]).x);
		}
		glEnd();
	}

}


void draw_model()
{
//	glutSolidTeapot(1);
//	glStaff::xyz_frame(1,1,1,false);
static int file_i=0, first=1;
	if( play_on || first ){
		first = 0;
		file_i = (file_i+1) % 800;
		char ss[100]; sprintf(ss, "%d", file_i);
		std::string file_name;
		file_name = std::string(DATA_FILE_PATH"rgb/") + ss + ".png";
		read_rgb(file_name.c_str(), img_rgb);
		file_name = std::string(DATA_FILE_PATH"depth/") + ss + ".dat";
		//file_name = "../data/MoG_bgmodel.dat";
		//read_depth(file_name.c_str(), img_depth);


		read_depth_by_gaoyu(file_name.c_str(), img_depth);


//		read_bgmodel( "../data/MoG_bgmodel.dat", img_rgb, img_depth );
//		cv::imwrite("bg_rgb.png", img_rgb);
//		img_depth = img_depth * (255/10000.0f);
//		cv::imwrite("bg_depth.png", img_depth);
		coor_img2cam(img_depth, img_3d);
		reconstruct(img_3d, tri_idxes);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_rgb.cols, img_rgb.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, img_rgb.data);

		std::cout << "img num: " << file_i << "\n";
	}

	if(true){
		GLboolean elt = glIsEnabled(GL_LIGHTING);
		GLboolean tex = glIsEnabled(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);

		float rgb[4]={0,1,0, 1};
		glBegin(GL_POINTS);
		for(int i=0; i<img_3d.rows; ++i)
			for(int j=0; j<img_3d.cols; ++j){
//				glStaff::hsl_to_rgb((img_3d.at<cv::Point3f>(i,j).y-1)/4*360,
//						1, 0.5f, rgb);
				glColor3fv(rgb);
				glVertex3fv(&img_3d.at<cv::Point3f>(i,j).x);
			}
		glEnd();

		if(elt) glEnable(GL_LIGHTING);
		if(tex) glEnable(GL_TEXTURE_2D);
	}else{
		GLfloat c[]={.0f, .0f, .0f, 1};
		glMaterialfv(GL_FRONT, GL_SPECULAR, c);
		c[0]=0.7f; c[1]=0.7f; c[2]=0.7f;
		glMaterialfv(GL_FRONT, GL_DIFFUSE, c);
		c[0]=2.0f; c[1]=2.0f; c[2]=2.0f;
		glMaterialfv(GL_FRONT, GL_AMBIENT, c);
		glBegin(GL_TRIANGLES);
		#define  POINT_1D(a)  (img_3d.at<cv::Point3f>((a)/img_3d.cols,(a)%img_3d.cols))
		float tx = 1.0f / img_3d.cols, ty = 1.0f / img_3d.rows;
		for(unsigned i=0; i<tri_idxes.size()/3; ++i){
			glm::vec3 n = glm::normalize(glm::cross(
					TO_GLM_VEC3(POINT_1D(tri_idxes[i*3+1]))-TO_GLM_VEC3(POINT_1D(tri_idxes[i*3])),
					TO_GLM_VEC3(POINT_1D(tri_idxes[i*3+2]))-TO_GLM_VEC3(POINT_1D(tri_idxes[i*3]))  ) );
			glNormal3fv(&n[0]);
			glTexCoord2f(tri_idxes[i*3]%img_3d.cols*tx,tri_idxes[i*3]/img_3d.cols*ty);
			glVertex3fv(&POINT_1D(tri_idxes[i*3]).x);
			glTexCoord2f(tri_idxes[i*3+1]%img_3d.cols*tx,tri_idxes[i*3+1]/img_3d.cols*ty);
			glVertex3fv(&POINT_1D(tri_idxes[i*3+1]).x);
			glTexCoord2f(tri_idxes[i*3+2]%img_3d.cols*tx,tri_idxes[i*3+2]/img_3d.cols*ty);
			glVertex3fv(&POINT_1D(tri_idxes[i*3+2]).x);
		}
		glEnd();
	}

}

void draw(const glm::mat4& mat_model, const glm::mat4& mat_view)
{
	double t = omp_get_wtime();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW); glLoadMatrixf(&mat_view[0][0]);
		draw_world();
	glMatrixMode(GL_MODELVIEW); glMultMatrixf(&mat_model[0][0]);
		draw_model_by_gaoyu();
	t = omp_get_wtime()-t;
	char st[50]; sprintf(st, "draw time (ms) :  %.2f", t*1000);
	glStaff::text_upperLeft(st, 1);
//	std::cout << t*1000 << "\n";
}

void mkey_p() { std::cout << "key p pressed\n"; }
void mkey_t()
{
	if(glIsEnabled(GL_TEXTURE_2D))
		glDisable(GL_TEXTURE_2D);
	else
		glEnable(GL_TEXTURE_2D);
}
void mkey_a() { play_on = !play_on; }
void mkey_m() { play_mode = !play_mode; }
void mkey_c() { translate_center = !translate_center;

glStaff::Internal::mat_view = glm::translate(glm::vec3(0,0,-100)) * glStaff::Internal::mat_view;//按下快捷键"C"时,将视角移远一些  Add by gaoyu 2015-7-29
}

void init_tex()
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_rgb.cols, img_rgb.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, img_rgb.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glEnable(GL_TEXTURE_2D);
}


//int main(void)
//{
//	glStaff::init_win(800, 800, "OpenGL", "");
//	glStaff::init_gl(); // have to be called after glStaff::init_win
//
//	glStaff::set_mat_view(
//		glm::lookAt( glm::vec3(0,5,-10), glm::vec3(0,0,0), glm::vec3(0,1,0) ) );
//	glStaff::set_mat_model(
//		glm::rotate(3.14f*0.17f, glm::vec3(1,0,0)) * glm::translate( glm::vec3(0,0,-5) ) );
//
//	glStaff::add_key_callback('P', mkey_p, L"print");
//	glStaff::add_key_callback('T', mkey_t, L"tex");
//	glStaff::add_key_callback('A', mkey_a, L"a");
//	glStaff::add_key_callback('M', mkey_m, L"a");
//
////	read_rgb("../data/rgb/209.jpg", img_rgb);
////	read_depth("../data/depth/209.dat", img_depth);
////	coor_img2cam(img_depth, img_3d);
////	reconstruct(img_3d, tri_idxes);
//	init_tex();
//	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
//
////	cv::imshow("rgb", img_rgb);
////	cv::imshow("depth", img_depth*(1.0f/10000) );
////	cv::waitKey(0);
//
//	glStaff::renderLoop(draw);
//}

//Add by gaoyu 2015-7-21
int main(void)
{
	//glStaff::init_win(800, 800, "OpenGL", "");
	glStaff::init_win(1024, 600, "OpenGL", "");//适应我保存的PNG文件数据 Add by gaoyu 2015-7-22


	glStaff::init_gl(); // have to be called after glStaff::init_win

	glStaff::set_mat_view(
		glm::lookAt( glm::vec3(0,5,-10), glm::vec3(0,0,0), glm::vec3(0,1,0) ) );
	glStaff::set_mat_model(
		glm::rotate(3.14f*0.17f, glm::vec3(1,0,0)) * glm::translate( glm::vec3(0,0,-5) ) );

	glStaff::add_key_callback('P', mkey_p, L"print");
	glStaff::add_key_callback('T', mkey_t, L"tex");
	glStaff::add_key_callback('A', mkey_a, L"a");
	glStaff::add_key_callback('M', mkey_m, L"a");
	glStaff::add_key_callback('C', mkey_c, L"c");

	glStaff::Internal::mat_view = glm::translate(glm::vec3(0,0,-50)) * glStaff::Internal::mat_view;//初始状态讲视角移动的远一些  Add by gaoyu 2015-8-4

//	read_rgb("../data/rgb/209.jpg", img_rgb);
//	read_depth("../data/depth/209.dat", img_depth);
//	coor_img2cam(img_depth, img_3d);
//	reconstruct(img_3d, tri_idxes);
	init_tex();
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

//	cv::imshow("rgb", img_rgb);
//	cv::imshow("depth", img_depth*(1.0f/10000) );
//	cv::waitKey(0);

	glStaff::renderLoop(draw);
}

//Add by gaoyu 2015-7-20
//int main(void)
//{
////	glStaff::init_win(800, 800, "OpenGL", "");
////	glStaff::init_gl(); // have to be called after glStaff::init_win
//
//	glStaff::set_mat_view(
//		glm::lookAt( glm::vec3(0,5,-10), glm::vec3(0,0,0), glm::vec3(0,1,0) ) );
//	glStaff::set_mat_model(
//		glm::rotate(3.14f*0.17f, glm::vec3(1,0,0)) * glm::translate( glm::vec3(0,0,-5) ) );
//
//	glStaff::add_key_callback('P', mkey_p, L"print");
//	glStaff::add_key_callback('T', mkey_t, L"tex");
//	glStaff::add_key_callback('A', mkey_a, L"a");
//	glStaff::add_key_callback('M', mkey_m, L"a");
//
//	read_rgb("../data/rgb/00000011.png", img_rgb);
//	read_depth_by_gaoyu("../data/depth/00000011.dat", img_depth);
//	cv::Mat norm;
//	cv::normalize(img_depth,norm,255,0,CV_MINMAX,CV_8UC1);//注意归一化到0~255很重要 Add by gaoyu 2015-7-20
////	coor_img2cam(img_depth, img_3d);
////	reconstruct(img_3d, tri_idxes);
//	init_tex();
//	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
//
//	cv::imshow("rgb", img_rgb);
////	cv::imshow("depth", img_depth*(1.0f/10000) );//注意最好不要用诚意倍数的方式归一化 Add by gaoyu 2015-7-20
//	cv::imshow("depth", norm );
//	cv::waitKey(0);
//
//	//glStaff::renderLoop(draw);
//}
