# Image content handlers sources

# S_IMAGE are sources related to image management
S_IMAGE_YES := image.c image_cache.c
S_IMAGE_NO :=
S_IMAGE_$(NETSURF_USE_BMP) += bmp.c
S_IMAGE_$(NETSURF_USE_GIF) += gif.c
S_IMAGE_$(NETSURF_USE_BMP) += ico.c
S_IMAGE_$(NETSURF_USE_JPEG) += jpeg.c
S_IMAGE_$(NETSURF_USE_JPEGXL) += jpegxl.c
S_IMAGE_$(NETSURF_USE_ROSPRITE) += nssprite.c
S_IMAGE_$(NETSURF_USE_PNG) += png.c
S_IMAGE_$(NETSURF_USE_NSSVG) += svg.c
S_IMAGE_$(NETSURF_USE_RSVG) += rsvg$(RSVG_API).c
S_IMAGE_$(NETSURF_USE_VIDEO) += video.c
S_IMAGE_$(NETSURF_USE_WEBP) += webp.c

S_IMAGE := $(S_IMAGE_YES)
