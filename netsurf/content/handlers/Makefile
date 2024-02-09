# Image content handler sources
include content/handlers/image/Makefile

S_IMAGE := $(addprefix content/handlers/image/,$(S_IMAGE))

# CSS sources
include content/handlers/css/Makefile

S_CONTENT += $(addprefix handlers/css/,$(S_CSS))

# Javascript source
include content/handlers/javascript/Makefile

S_CONTENT += $(addprefix handlers/javascript/,$(S_JAVASCRIPT))

# HTML content handler sources
include content/handlers/html/Makefile

S_CONTENT += $(addprefix handlers/html/,$(S_HTML))

# Text content handler sources
include content/handlers/text/Makefile

S_CONTENT += $(addprefix handlers/text/,$(S_TEXT))

# extend the include search path
INCLUDE_DIRS += content/handlers
