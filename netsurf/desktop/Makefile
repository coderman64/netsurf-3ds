# Sources for desktop

S_DESKTOP := cookie_manager.c knockout.c hotlist.c mouse.c		\
	plot_style.c print.c search.c searchweb.c scrollbar.c		\
	textarea.c version.c system_colour.c		\
	local_history.c global_history.c treeview.c page-info.c

S_DESKTOP := $(addprefix desktop/,$(S_DESKTOP))

# version.c needs the testament
desktop/version.c: testament $(OBJROOT)/testament.h

# S_BROWSER are sources related to full browsers but are common
# between RISC OS, GTK, BeOS and AmigaOS builds
S_BROWSER := bitmap.c browser.c browser_window.c browser_history.c \
	download.c frames.c netsurf.c cw_helper.c \
	save_complete.c save_text.c selection.c textinput.c gui_factory.c \
	save_pdf.c font_haru.c

S_BROWSER := $(addprefix desktop/,$(S_BROWSER))
