/*
 * appsrc.h
 *
 *  Created on: 2020年1月20日
 *      Author: alex
 */

#ifndef APPSRC_H_
#define APPSRC_H_

#include <stdio.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include <opencv2/opencv.hpp>

typedef struct {
	GstPipeline *pipeline;
	GstAppSrc  *src;
	GstElement *filter1;
	GstElement *encoder;
	GstElement *filter2;
	GstElement *parser;
	GstElement *qtmux;
	GstElement *sink;

	GstElement *convert;

	GstClockTime timestamp;
	guint sourceid;
} gst_app_t;


int gstInit();
void pushData(cv::Mat inFrame);


#endif /* APPSRC_H_ */
