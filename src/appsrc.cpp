/*
 * appsrc.cpp
 *
 *  Created on: 2020年1月20日
 *      Author: alex
 */


#include "appsrc.h"

static gst_app_t gst_app;

using namespace cv;

void pushData(Mat inFrame)
{
	gst_app_t *app = &gst_app;

	//Push the data from buffer to gstpipeline 100 times

	unsigned char* filebuffer = (unsigned char*)inFrame.data;
	if (filebuffer == NULL)
	{
		printf("Memory error\n");
		exit (2);
	}//Errorcheck

	GstBuffer *pushbuffer; //Actual databuffer
	GstFlowReturn ret; //Return value
	pushbuffer = gst_buffer_new_wrapped (filebuffer, inFrame.cols*inFrame.rows*inFrame.channels()); //Wrap the data

	//Set frame timestamp
	GST_BUFFER_PTS      (pushbuffer) = app->timestamp;
	GST_BUFFER_DTS      (pushbuffer) = app->timestamp;
	GST_BUFFER_DURATION (pushbuffer) = gst_util_uint64_scale_int (1, GST_SECOND, 1);
	app->timestamp += GST_BUFFER_DURATION (pushbuffer);
	//printf("Frame is at %lu\n", app->timestamp);

	ret = gst_app_src_push_buffer( app->src, pushbuffer); //Push data into pipeline

	g_assert(ret ==  GST_FLOW_OK);

	return ;
}


int gstInit()
{
	gst_app_t *app = &gst_app;

	GstStateChangeReturn state_ret;
	app->timestamp = 0; //Set timestamp to 0
	gst_init(NULL, NULL);

	//Create pipeline, and pipeline elements
	app->pipeline = (GstPipeline*)gst_pipeline_new("mypipeline");
	app->src    =   (GstAppSrc*)gst_element_factory_make("appsrc", "mysrc");
	app->filter1 =  gst_element_factory_make ("capsfilter", "myfilter1");
	app->encoder =  gst_element_factory_make ("omxh264enc", "myomx");
	app->filter2 =  gst_element_factory_make ("capsfilter", "myfilter2");
	app->parser =   gst_element_factory_make("h264parse"  , "myparser");
	app->qtmux =    gst_element_factory_make("qtmux"      , "mymux");
	app->sink =     gst_element_factory_make ("filesink"  , NULL);

	if(	!app->pipeline ||
		!app->src      || !app->filter1 ||
		!app->encoder  || !app->filter2 ||
		!app->parser   || !app->qtmux    ||
		!app->sink    )  {
		printf("Error creating pipeline elements!\n");
		exit(2);
	}

	gst_bin_add_many (GST_BIN(app->pipeline),
			(GstElement*)app->src,app->filter1,
			app->encoder,app->filter2,
			app->parser,app->qtmux,
			app->sink, NULL);


	//Set pipeline element attributes
	g_object_set (app->src, "format", GST_FORMAT_TIME, NULL);
	GstCaps *filtercaps1 = gst_caps_new_simple ("video/x-raw",
		"format", G_TYPE_STRING, "YUYV",
		"width", G_TYPE_INT, 1920*2,
		"height", G_TYPE_INT, 1080,
		"framerate", GST_TYPE_FRACTION, 30, 1,
		NULL);
	g_object_set (G_OBJECT (app->filter1), "caps", filtercaps1, NULL);
	GstCaps *filtercaps2 = gst_caps_new_simple ("video/x-h264",
		"stream-format", G_TYPE_STRING, "byte-stream",
		NULL);
	g_object_set (G_OBJECT (app->filter2), "caps", filtercaps2, NULL);
	g_object_set (G_OBJECT (app->sink), "location", "/home/nvidia/output.h264", NULL);

	//Link elements together
	g_assert( gst_element_link_many(
		(GstElement*)app->src,
		app->filter1,
		app->encoder,
		app->filter2,
		app->parser,
		app->qtmux,
		app->sink,
		NULL ) );

	//Play the pipeline
	state_ret = gst_element_set_state((GstElement*)app->pipeline, GST_STATE_PLAYING);
	g_assert(state_ret == GST_STATE_CHANGE_SUCCESS);


	return 0;
}
