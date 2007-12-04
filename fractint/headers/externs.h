#ifndef EXTERNS_H
#define EXTERNS_H

/* keep var names in column 30 for sorting via sort /+30 <in >out */
extern int					g_adapter;							/* index into g_video_table[] */
extern float				g_aspect_drift;
extern int					g_atan_colors;
extern int					g_auto_stereo_depth;
extern double				g_auto_stereo_width;
extern bool					g_bad_outside;
extern long					g_bail_out;
extern int					(*g_bail_out_fp)();
extern int					(*g_bail_out_l)();
extern int					(*g_bail_out_bn)();
extern int					(*g_bail_out_bf)();
extern enum bailouts		g_bail_out_test;
extern int					g_basin;
extern int					g_bf_save_len;
extern int					g_bf_digits;
extern int					g_biomorph;
extern int					g_bit_shift;
extern int					g_bit_shift_minus_1;
extern BYTE					g_block[];
extern long					g_calculation_time;
extern long					(*g_calculate_mandelbrot_asm_fp)();
extern int					(*g_calculate_type)();
extern int					g_calculation_status;
extern StereogramCalibrateType	g_stereogram_calibrate;
extern int					g_cfg_line_nums[];
extern bool					g_check_current_dir;
extern long					g_c_imag;
extern double				g_close_enough;
extern double				g_proximity; // escape time parameter for epscross and fmod coloring methods
extern ComplexD				g_coefficient;
extern int					g_col;
extern int					g_color;
extern long					g_color_iter;
extern bool					g_color_preloaded;
extern int					g_colors;
extern int					g_color_state;
extern int					g_color_bright;					/* brightest color in palette */
extern int					g_color_dark;					/* darkest color in palette */
extern int					g_color_medium;					/* nearest to medbright grey in palette */
extern bool					g_compare_gif;
extern long					g_gaussian_constant;
extern double				g_cos_x;
extern long					g_c_real;
extern int					g_current_col;
extern int					g_current_pass;
extern int					g_current_row;
extern int					g_cycle_limit;
extern int					g_c_exp;
extern BYTE					g_dac_box[256][3];
extern int					g_dac_count;
extern double				g_delta_min_fp;
extern int					g_debug_mode;
extern int					g_decimals;
extern BYTE					g_decoder_line[];
extern int					g_decomposition[];
extern int					g_degree;
extern long					g_delta_min;
extern float				g_depth_fp;
extern bool					g_disk_16bit;
extern bool					g_disk_flag;						/* disk video active flag */
extern bool					g_disk_targa;
extern int					g_display_3d;
extern long					g_distance_test;
extern int					g_distance_test_width;
extern float				g_screen_distance_fp;
extern int					g_gaussian_distribution;
extern bool					g_dither_flag;
extern bool					g_dont_read_color;
extern double				g_delta_parameter_image_x;
extern double				g_delta_parameter_image_y;
extern BYTE					g_stack[];
extern double				(*g_dx_pixel)(); /* set in FRACTALS.C */
extern double				g_dx_size;
extern double				(*g_dy_pixel)(); /* set in FRACTALS.C */
extern double				g_dy_size;
extern bool					g_escape_exit_flag;
extern int					g_evolving_flags;
extern evolution_info		*g_evolve_handle;
extern float				g_eyes_fp;
extern bool					g_fast_restore;
extern double				g_fudge_limit;
extern long					g_one_fudge;
extern long					g_two_fudge;
extern int					g_grid_size;
extern double				g_fiddle_factor;
extern double				g_fiddle_reduction;
extern float				g_file_aspect_ratio;
extern int					g_file_colors;
extern int					g_file_x_dots;
extern int					g_file_y_dots;
extern int					g_fill_color;
extern float				g_final_aspect_ratio;
extern int					g_finish_row;
extern bool					g_command_initialize;
extern int					g_first_saved_and;
extern bool					g_float_flag;
extern ComplexD				*g_float_parameter;
extern const BYTE			g_font_8x8[8][1024/8];
extern int					g_force_symmetry;
extern int					g_fractal_type;
extern bool					g_from_text_flag;
extern long					g_fudge;
extern FunctionListItem		g_function_list[];
extern bool					g_function_preloaded;
extern double				g_f_radius;
extern double				g_f_x_center;
extern double				g_f_y_center;
extern GENEBASE				g_genes[NUMGENES];
extern bool					g_gif87a_flag;
extern int					g_good_mode;						/* video mode ok? */
extern bool					g_got_real_dac;					/* loaddac worked, really got a dac */
extern int					g_got_status;
extern bool					g_grayscale_depth;
extern bool					g_has_inverse;
extern unsigned int			g_height;
extern float				g_height_fp;
extern float				*g_ifs_definition;
extern int					g_ifs_type;
extern bool					g_image_map;
extern ComplexD				g_initial_z;
extern int					g_initialize_batch;
extern int					g_initial_cycle_limit;
extern int					g_initial_adapter;
extern ComplexD				g_initial_orbit_z;
extern int					g_save_time;
extern int					g_inside;
extern int					g_integer_fractal;
extern double				g_inversion[];
extern int					g_invert;
extern int					g_is_true_color;
extern bool					g_is_mandelbrot;
extern int					g_x_stop;
extern int					g_y_stop;
extern double				g_julia_c_x;
extern double				g_julia_c_y;
extern int					g_juli_3d_mode;
extern const char			*g_juli_3d_options[];
extern bool					g_julibrot;
extern int					g_input_counter;
extern bool					g_keep_screen_coords;
extern int					g_last_orbit_type;
extern long					g_close_enough_l;
extern ComplexL				g_coefficient_l;
extern bool					g_use_old_complex_power;
extern BYTE					*g_line_buffer;
extern ComplexL				g_initial_z_l;
extern ComplexL				g_init_orbit_l;
extern long					g_initial_x_l;
extern long					g_initial_y_l;
extern long					g_limit2_l;
extern long					g_limit_l;
extern long					g_magnitude_l;
extern ComplexL				g_new_z_l;
extern int					g_loaded_3d;
extern int					g_lod_ptr;
extern bool					g_log_automatic_flag;
extern bool					g_log_calculation;
extern int					g_log_dynamic_calculate;
extern long					g_log_palette_mode;
extern BYTE					*g_log_table;
extern ComplexL				g_old_z_l;
extern ComplexL				*g_long_parameter;
extern ComplexL				g_parameter2_l;
extern ComplexL				g_parameter_l;
extern long					g_temp_sqr_x_l;
extern long					g_temp_sqr_y_l;
extern ComplexL				g_tmp_z_l;
extern long					(*g_lx_pixel)(); /* set in FRACTALS.C */
extern long					(*g_ly_pixel)(); /* set in FRACTALS.C */
extern void					(*g_trig0_l)();
extern void					(*g_trig1_l)();
extern void					(*g_trig2_l)();
extern void					(*g_trig3_l)();
extern void					(*g_trig0_d)();
extern void					(*g_trig1_d)();
extern void					(*g_trig2_d)();
extern void					(*g_trig3_d)();
extern double				g_magnitude;
extern unsigned long		g_magnitude_limit;
extern MajorMethodType			g_major_method;
extern BYTE					*g_map_dac_box;
extern bool					g_map_set;
extern int					g_math_error_count;
extern double				g_math_tolerance[2];
extern int					g_max_color;
extern long					g_max_count;
extern long					g_max_iteration;
extern int					g_max_line_length;
extern long					g_max_log_table_size;
extern long					g_bn_max_stack;
extern int					g_max_colors;
extern int					g_max_input_counter;
extern int					g_max_history;
extern int					g_cross_hair_box_size;
extern MinorMethodType			g_minor_method;
extern more_parameters		g_more_parameters[];
extern int					g_overflow_mp;
extern double				g_m_x_max_fp;
extern double				g_m_x_min_fp;
extern double				g_m_y_max_fp;
extern double				g_m_y_min_fp;
extern int					g_name_stack_ptr;
extern ComplexD				g_new_z;
extern int					g_new_discrete_parameter_offset_x;
extern int					g_new_discrete_parameter_offset_y;
extern double				g_new_parameter_offset_x;
extern double				g_new_parameter_offset_y;
extern int					g_new_orbit_type;
extern int					g_next_saved_incr;
extern bool					g_no_magnitude_calculation;
extern bool					g_no_bof;
extern int					g_num_affine;
extern unsigned				g_num_colors;
extern const int			g_num_function_list;
extern int					g_num_fractal_types;
extern bool					g_next_screen_flag;
extern int					g_gaussian_offset;
extern bool					g_ok_to_print;
extern ComplexD				g_old_z;
extern long					g_old_color_iter;
extern BYTE					g_old_dac_box[256][3];
extern bool					g_old_demm_colors;
extern int					g_discrete_parameter_offset_x;
extern int					g_discrete_parameter_offset_y;
extern double				g_parameter_offset_x;
extern double				g_parameter_offset_y;
extern int					g_orbit_save;
extern int					g_orbit_color;
extern int					g_orbit_delay;
extern int					g_orbit_draw_mode;
extern long					g_orbit_interval;
extern int					g_orbit_index;
extern bool					g_organize_formula_search;
extern float				g_origin_fp;
extern int					(*g_out_line) (BYTE *, int);
extern void					(*g_out_line_cleanup)();
extern int					g_outside;
extern bool					g_overflow;
extern int					g_overlay_3d;
extern bool					g_fractal_overwrite;
extern double				g_orbit_x_3rd;
extern double				g_orbit_x_max;
extern double				g_orbit_x_min;
extern double				g_orbit_y_3rd;
extern double				g_orbit_y_max;
extern double				g_orbit_y_min;
extern double				g_parameters[];
extern double				g_parameter_range_x;
extern double				g_parameter_range_y;
extern double				g_parameter_zoom;
extern ComplexD				g_parameter2;
extern ComplexD				g_parameter;
extern int					g_passes;
extern int					g_patch_level;
extern int					g_periodicity_check;
extern void					(*g_plot_color)(int,int,int);
extern double				g_plot_mx1;
extern double				g_plot_mx2;
extern double				g_plot_my1;
extern double				g_plot_my2;
extern bool					g_potential_16bit;
extern bool					g_potential_flag;
extern double				g_potential_parameter[];
#ifndef XFRACT
extern U16					prefix[];
#endif
extern int					g_px;
extern int					g_py;
extern int					g_parameter_box_count;
extern int					g_pseudo_x;
extern int					g_pseudo_y;
extern void					(*g_plot_color_put_color)(int,int,int);
extern ComplexD				g_power;
extern double				g_quaternion_c;
extern double				g_quaternion_ci;
extern double				g_quaternion_cj;
extern double				g_quaternion_ck;
extern bool					g_quick_calculate;
extern int					*g_ranges;
extern int					g_ranges_length;
extern long					g_real_color_iter;
extern int					g_release;
extern int					g_resave_mode;
extern bool					g_reset_periodicity;
extern char					*g_resume_info;
extern int					g_resume_length;
extern bool					g_resuming;
extern bool					g_use_fixed_random_seed;
extern char					g_rle_buffer[];
extern ComplexD				*g_roots;
extern int					g_rotate_hi;
extern int					g_rotate_lo;
extern int					g_row;
extern int					g_row_count;						/* row-counter for decoder and out_line */
extern double				g_rq_limit2;
extern double				g_rq_limit;
extern int					g_random_seed;
extern long					g_save_base;
extern ComplexD				g_save_c;
extern int					g_save_dac;
extern long					g_save_ticks;
extern int					g_save_release;
extern float				g_screen_aspect_ratio;
extern int					g_screen_height;
extern int					g_screen_width;
extern bool					g_set_orbit_corners;
extern int					g_show_dot;
extern int					g_show_file;
extern bool					g_show_orbit;
extern double				g_sin_x;
extern int					g_size_dot;
extern short				g_size_of_string[];
extern short				g_skip_x_dots;
extern short				g_skip_y_dots;
extern int					g_slides;
extern int					g_gaussian_slope;
extern void					(*g_plot_color_standard)(int x, int y, int color);
extern bool					g_start_show_orbit;
extern bool					g_started_resaves;
extern ComplexD				g_static_roots[];
extern char					g_stereo_map_name[];
extern int					g_stop_pass;
extern unsigned int			g_string_location[];
extern BYTE					g_suffix[];
extern double				g_sx_3rd;
extern double				g_sx_max;
extern double				g_sx_min;
extern int					g_sx_offset;
extern double				g_sy_3rd;
extern double				g_sy_max;
extern double				g_sy_min;
extern SymmetryType			g_symmetry;
extern int					g_sy_offset;
extern bool					g_tab_display_enabled;
extern bool					g_targa_output;
extern bool					g_targa_overlay;
extern double				g_temp_sqr_x;
extern double				g_temp_sqr_y;
extern int					g_text_cbase;						/* g_text_col is relative to this */
extern int					g_text_col;						/* current column in text mode */
extern BYTE					g_text_colors[];
extern int					g_text_rbase;						/* g_text_row is relative to this */
extern int					g_text_row;						/* current row in text mode */
extern unsigned int			g_this_generation_random_seed;
extern bool					g_three_pass;
extern double				g_threshold;
extern TimedSaveType		g_timed_save;
extern bool					g_timer_flag;
extern long					g_timer_interval;
extern long					g_timer_start;
extern ComplexD				g_temp_z;
extern double				g_too_small;
extern int					g_total_passes;
extern int					g_function_index[];
extern bool					g_true_color;
extern bool					g_true_mode_iterates;
extern char					g_text_stack[];
extern double				g_two_pi;
extern UserInterfaceState	g_ui_state;
extern InitialZType			g_use_initial_orbit_z;
extern bool					g_use_center_mag;
extern bool					g_use_old_periodicity;
extern bool					g_using_jiim;
extern int					g_user_biomorph;
extern long					g_user_distance_test;
extern bool					g_user_float_flag;
extern int					g_user_periodicity_check;
extern VIDEOINFO			g_video_entry;
extern VIDEOINFO			g_video_table[];
extern int					g_video_table_len;
extern int					g_video_type;						/* video adapter type */
extern VECTOR				g_view;
extern bool					g_view_crop;
extern float				g_view_reduction;
extern bool					g_view_window;
extern int					g_view_x_dots;
extern int					g_view_y_dots;
extern int					g_vx_dots;
extern int					g_which_image;
extern float				g_width_fp;
extern int					g_x_dots;
extern int					g_x_shift1;
extern int					g_x_shift;
extern int					g_xx_adjust1;
extern int					g_xx_adjust;
extern long					g_xx_one;
extern int					g_y_dots;
extern int					g_y_shift;
extern int					g_y_shift1;
extern int					g_yy_adjust;
extern int					g_yy_adjust1;
extern double				g_zbx;
extern double				g_zby;
extern double				g_z_depth;
extern int					g_z_dots;
extern int					g_z_rotate;
extern double				g_z_skew;
extern double				g_z_width;
extern bool					g_zoom_off;

#ifdef XFRACT
extern  int					g_fake_lut;
#endif

#endif
