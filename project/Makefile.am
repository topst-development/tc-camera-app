ACLOCAL_AMFLAGS = -I m4
CC = @CC@
CPP = @CPP@
AM_CFLAGS = @CFLAGS@ \
	-I@srcdir@/common \
	-I@srcdir@/hal \
	-I@srcdir@/hal/switch \
	-I@srcdir@/hal/mcu_manager \
	-I@srcdir@/hal/v4l2 \
	-I@srcdir@/hal/overlay \
	-I@srcdir@/hal/cam_ipc \
	-I@srcdir@/framework \
	-I@srcdir@/framework/video_input \
	-I@srcdir@/framework/video_output \
	-I@srcdir@/app \
	-I@srcdir@/app/camera \
	-I@KERNEL_DIR@/include/uapi \
	-I@KERNEL_DIR@/include \
	-I@KERNEL_DIR@/include/video/tcc
AM_CPPFLAGS = $(AM_CFLAGS)
LIBS = @LIBS@
DEFS = @DEFS@

bin_PROGRAMS = camera_app
camera_app_SOURCES = \
	common/klog.c \
	common/v4l2.c \
	common/message_queue.c \
	hal/switch/switch.c \
	hal/v4l2/v4l2_capture.c \
	hal/overlay/overlay.c \
	hal/cam_ipc/cam_ipc.c \
	framework/video_input/video_input.c \
	framework/video_output/video_output.c \
	app/camera/camera.c \
	main.c

#	hal/mcu_manager/cm4_manager.c
#	hal/g2d/g2d.c
