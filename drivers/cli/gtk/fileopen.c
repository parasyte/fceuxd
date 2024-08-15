#include "gui.h"
#include "fileopen.h"

static  GtkWidget *filew;
extern char *gui_lastpath;

static void file_ok_sel( GtkWidget *w, GtkFileSelection *fs )
{
 char *path=gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));
 if(LoadGame(path))
 {
  if(gui_lastpath)
   free(gui_lastpath);
  gui_lastpath=malloc(strlen(path)+1);
  strcpy(gui_lastpath,path);
  gtk_widget_destroy(filew);
 }
}

void gui_fileopen( GtkWidget *w, gpointer   data )
{

    /* Create a new file selection widget */
    filew = gtk_file_selection_new ("File selection");
    
//    gtk_signal_connect (GTK_OBJECT (filew), "destroy",
//			(GtkSignalFunc) destroy, &filew);
    /* Connect the ok_button to file_ok_sel function */
    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filew)->ok_button),
			"clicked", (GtkSignalFunc) file_ok_sel, filew );
    
    /* Connect the cancel_button to destroy the widget */
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION
                                            (filew)->cancel_button),
			       "clicked", (GtkSignalFunc) gtk_widget_destroy,
			       GTK_OBJECT (filew));
    
    /* Lets set the filename, as if this were a save dialog, and we are giving
     a default filename */
    gtk_file_selection_set_filename (GTK_FILE_SELECTION(filew), 
				     gui_lastpath);
    
    gtk_widget_show(filew);
}
