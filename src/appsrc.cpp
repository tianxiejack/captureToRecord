/*
 * appsrc.cpp
 *
 *  Created on: 2020年1月20日
 *      Author: alex
 */


#include "appsrc.h"

static gst_app_t gst_app;

using namespace cv;
using namespace cr_osa;

//OSA_BufHndl gBufhandle;

extern Mat gFullMat;
extern OSA_BufHndl tskfirstQuene;

void pushData(Mat inFrame)
{

	return ;
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

static void cb_need_data (GstElement *appsrc,guint unused_size,gpointer user_data)
{
	static gboolean white = FALSE;
	static GstClockTime timestamp = 0;
	GstBuffer *buffer;
	guint size;
	GstFlowReturn ret;

	int bufId;
	int osaret = OSA_bufGetFull(&tskfirstQuene, &bufId, 100*1000);

	if(osaret == -1)
		return;

	Mat colorframe = Mat(1080,1920*2,CV_8UC3,tskfirstQuene.bufInfo[bufId].virtAddr);

#if 1

	gst_app_t *app = &gst_app;
	GstMapInfo map;
	gst_buffer_map(buffer,&map,GST_MAP_READ);
	memcpy(map.data,colorframe.data,colorframe.cols*colorframe.rows*colorframe.channels());
	GST_BUFFER_PTS (buffer) = timestamp;
	GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 25);
	timestamp += GST_BUFFER_DURATION (buffer) ;
	g_signal_emit_by_name (app->src, "push-buffer", buffer, &ret);
	gst_buffer_unmap(buffer,&map);
	gst_buffer_unref (buffer);

#else

  size = 385 * 288 * 2;

  buffer = gst_buffer_new_allocate (NULL, size, NULL);

  /* this makes the image black/white */
  gst_buffer_memset (buffer, 0, white ? 0xff : 0x0, size);

  white = !white;

  GST_BUFFER_PTS (buffer) = timestamp;
  GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 2);

  timestamp += GST_BUFFER_DURATION (buffer);
  g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);
  gst_buffer_unref (buffer);

#endif

  if (ret != GST_FLOW_OK) {
    /* something wrong, stop pushing */
   // g_main_loop_quit (loop);
  }

}


int gstInit()
{
	//OSA_bufCreate(&gBufhandle, OSA_BufCreate *bufInit)

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
	app->convert =  gst_element_factory_make("nvvidconv", "myconvert");

	if(	!app->pipeline ||
		!app->src      || !app->filter1 ||
		!app->encoder  || !app->filter2 ||
		!app->parser   || !app->qtmux    ||
		!app->sink     || !app->convert )  {
		printf("Error creating pipeline elements!\n");
		exit(2);
	}

	gst_bin_add_many (GST_BIN(app->pipeline),
			(GstElement*)app->src,
			//app->filter1,
			app->convert,
			app->encoder,app->filter2,
			app->parser,app->qtmux,
			app->sink,
			NULL);


	//Set pipeline element attributes
	g_object_set (app->src, "format", GST_FORMAT_TIME, NULL);
	GstCaps *filtercaps1 = gst_caps_new_simple ("video/x-raw",
		"format", G_TYPE_STRING, "RGB",//"YUY2",
		"width", G_TYPE_INT, 1920*2,
		"height", G_TYPE_INT, 1080,
		"framerate", GST_TYPE_FRACTION, 0, 1,
		NULL);
	g_object_set (G_OBJECT (app->src), "caps", filtercaps1, NULL);

	g_signal_connect (app->src, "need-data", G_CALLBACK (cb_need_data), NULL);


	  GstCaps *filtercaps2 = gst_caps_new_simple ("video/x-h264",
		"stream-format", G_TYPE_STRING, "byte-stream",
		NULL);
	g_object_set (G_OBJECT (app->filter2), "caps", filtercaps2, NULL);
	g_object_set (G_OBJECT (app->sink), "location", "/home/nvidia/output.h264", NULL);


//	GstCaps *convertcaps = gst_caps_new_simple ("video/x-raw(memory:NVMM)",
//		"format", G_TYPE_STRING, "I420",NULL);
//	g_object_set(G_OBJECT(app->convert),"caps",convertcaps , NULL);


	//Link elements together
	g_assert( gst_element_link_many(
		(GstElement*)app->src,
		//app->filter1,
		app->convert,
		app->encoder,
		app->filter2,
		app->parser,
		app->qtmux,
		app->sink,
		NULL ) );

	//Play the pipeline
	state_ret = gst_element_set_state((GstElement*)app->pipeline, GST_STATE_PLAYING);
	g_assert(state_ret == GST_STATE_CHANGE_ASYNC);


	return 0;
}
