/******** examples/interfs.cc ***********/

//
// Initializers are below for different tools. Comment or uncomment to experiment
// with specific ones!
//
//


//#include <lax/interfaces/bezinterface.h>
//#include <lax/interfaces/linesinterface.h>
//#include <lax/interfaces/freehandinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/linestyle.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/rectinterface.h>
#include <lax/interfaces/objectinterface.h>
#include <lax/interfaces/captioninterface.h>
#include <lax/interfaces/viewerwindow.h>
#include <lax/interfaces/viewportwithstack.h>
#include <lax/units.h>


using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;



//----------------------------- main() ------------------------------
int main(int argc,char **argv)
{
	anXApp app;
	app.Backend("xlib");
	//app.Backend("cairo");
	app.Theme("Dark");

	app.init(argc,argv);

	 // owner and dp get assigned in PathInterface constructor
	ViewportWithStack vp(NULL,"vw viewportwithstack","viewportwithstack",
								  ANXWIN_HOVER_FOCUS|VIEWPORT_BACK_BUFFER|VIEWPORT_ROTATABLE,
								  0,0,0,0,0
								 );
	ViewerWindow *viewer=new ViewerWindow(NULL,"Viewer","Viewer",
			ANXWIN_CENTER|ANXWIN_ESCAPABLE,
			100,100, 600,500, 5,
			&vp
		);


	 //add to selection of tools
	//viewer->AddTool(new     CaptionInterface(100,NULL,"New text\nline 2\n  spaced line 3"), 1,0);
	//viewer->AddTool(new     ObjectInterface(100,NULL), 1,0);
	//viewer->AddTool(new   GradientInterface(101,NULL), 1,0);
	//viewer->AddTool(new      ImageInterface(102,NULL), 1,0);
	//viewer->AddTool(new      PatchInterface(103,NULL), 1,0);
	//viewer->AddTool(new ColorPatchInterface(104,NULL), 1,0);
	//viewer->AddTool(new       RectInterface(105,NULL), 1,0);
	viewer->AddTool(new       PathInterface(106,NULL), 1,0);
	//viewer->AddTool(new        BezInterface(107,NULL), 1,0);
	//viewer->AddTool(new   FreehandInterface(108,NULL), 1,0);
	//viewer->AddTool(new    EllipseInterface(109,NULL), 1,0);
	//viewer->AddTool(new    MeasureInterface(110,NULL), 1,0);

	//PathsData *path=SvgToPathsData(NULL,"m 3.3880181,539.58699 -29.2944001,0 -0.1786,20.3309 m 5.2294,21.23339 0,17.4124 24.2436001,0 m -0.1785,-38.64579 -45.4569001,0 0.1786,21.23339 45.4568001,0 m 0,-57.48939 0,89.99999", NULL);
	//vp.DropObject(path, 50,100);

	//viewer->SelectTool(106);
	app.addwindow(viewer);

	
	app.run();
	cout <<"---------App Close--------------"<<endl;
	app.close();
	cout <<"---------Bye!--------------"<<endl;
	return 0;
}


