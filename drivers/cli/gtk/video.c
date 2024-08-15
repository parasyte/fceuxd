#include "gui.h"
#include "video.h"

#define SCONF_ENTRY	0
#define SCONF_RADIO	1
#define SCONF_CHECK	2

typedef struct {
	char *label;
	int type;
	GtkWidget *widget;
	void *data;
} SCONF;

void FetchSettings(SCONF *conf)
{
 while(*conf->label)
 {
  switch(conf->type)
  {
   case SCONF_ENTRY:
  }
  conf++;
 }
}

void MakeSettingsTable(SCONF *conf, GtkWidget *container)
{
 GtkWidget *table1;

 table1=gtk_table_new(5,2,1);
 gtk_container_add(GTK_CONTAINER(container), table1);
 
 {
  int x;
  char *labels[]={"X Resolution:","Y Resolution:", "Bits Per Pixel:","X Scale:",
                  "Y Scale:"};
  for(x=0;x<5;x++)
  {
   GtkWidget *tmp;
   tmp=gtk_label_new(labels[x]);
   gtk_table_attach_defaults(table1,tmp,0,1,x,x+1);
   gtk_widget_show(tmp);
  }
 }
 gtk_widget_show(table1);

}

static GtkWidget *videowindow;

void gui_videoconfig( GtkWidget *w, gpointer data)
{
 GtkWidget *vbox1,*hbox1,*frame1,*frame2,*frame3,*table1;

 videowindow=gtk_window_new(GTK_WINDOW_TOPLEVEL);
 gtk_container_set_border_width(GTK_CONTAINER(videowindow),1);
 gtk_window_set_title(GTK_WINDOW(videowindow),"Video Configuration");

 vbox1=gtk_vbox_new(FALSE,2);
 gtk_container_set_border_width(GTK_CONTAINER(vbox1), 1);
 gtk_container_add(GTK_CONTAINER(videowindow), vbox1);

 hbox1=gtk_hbox_new(FALSE,2);
 gtk_container_set_border_width(GTK_CONTAINER(hbox1), 1);
 gtk_box_pack_start(GTK_BOX(vbox1),hbox1,FALSE,TRUE,0);
 gtk_widget_show(hbox1);
 gtk_widget_show(vbox1);

 frame1=gtk_frame_new("Full Screen");
 frame2=gtk_frame_new("Windowed");
 frame3=gtk_frame_new("Common Settings");

 gtk_box_pack_start(GTK_BOX(hbox1),frame1,FALSE,TRUE,0);
 gtk_box_pack_start(GTK_BOX(hbox1),frame2,FALSE,TRUE,0);
 gtk_box_pack_start(GTK_BOX(vbox1),frame3,FALSE,TRUE,0);
 gtk_widget_show(frame1);
 gtk_widget_show(frame2);
 gtk_widget_show(frame3);
 

 table1=gtk_table_new(5,2,1);
 gtk_container_add(GTK_CONTAINER(frame1), table1);
 
 {
  int x;
  char *labels[]={"X Resolution:","Y Resolution:", "Bits Per Pixel:","X Scale:",
		  "Y Scale:"};
  for(x=0;x<5;x++)
  {
   GtkWidget *tmp;
   tmp=gtk_label_new(labels[x]);
   gtk_table_attach_defaults(table1,tmp,0,1,x,x+1);
   gtk_widget_show(tmp);
  }
 } 
 gtk_widget_show(table1);
 gtk_widget_show(videowindow);
}
