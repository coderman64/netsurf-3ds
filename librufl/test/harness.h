#ifndef rufl_test_harness_h_
#define rufl_test_harness_h_

void rufl_test_harness_init(int fm_version, bool fm_ucs, bool preload);
void rufl_test_harness_register_font(const char *name);
void rufl_test_harness_register_encoding(const char *encoding);
void rufl_test_harness_set_font_encoding(const char *fontname,
		const char *encoding, const char *path);

#endif
