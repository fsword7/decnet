/*	CTERM structures
	Author:			Eduardo Marcelo Serrat
*/

#define	CTRL_X	0x18
#define CTRL_U	0x17
#define CTRL_O	0x0F
#define CTRL_R	0x12
#define CTRL_W	0x17
#define CTRL_S	0x13
#define	CTRL_Q  0x11
#define CTRL_Y	0x19
#define CTRL_C	0x03
#define	CTRL_T	0x14
#define DEL	0x7F
#define BS	0x08
#define ESC	0x1B

static	char	BELL=0x07;
struct	logical_terminal_characteristics
{
	short			mode_writing_allowed;
	int			terminal_attributes;
	char			terminal_type[6];
	short			output_flow_control;
	short			output_page_stop;
	short			flow_character_pass_through;
	short			input_flow_control;
	short			loss_notification;
	int			line_width;
	int			page_length;
	int			stop_length;
	int			cr_fill;
	int			lf_fill;
	int			wrap;
	int			horizontal_tab;
	int			vertical_tab;
	int			form_feed;
};

struct	physical_terminal_characteristics
{
	int			input_speed;
	int			output_speed;
	int			character_size;
	short			parity_enable;
	int			parity_type;
	short			modem_present;
	short			auto_baud_detect;
	short			management_guaranteed;
	char			switch_char_1;
	char			switch_char_2;
	short			eigth_bit;
	short			terminal_management_enabled;
};

struct	handler_maintained_characteristics
{
	short			ignore_input;
	short			control_o_pass_through;
	short			raise_input;
	short			normal_echo;
	short			input_escseq_recognition;
	short			output_escseq_recognition;
	int			input_count_state;
	short			auto_prompt;
	short			error_processing;
};

