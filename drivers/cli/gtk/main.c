#include "gui.h"
#include "fileopen.h"
#include "video.h"

char *gui_lastpath=0;
CFGSTRUCT GUIConfig[]={
	ACS(gui_lastpath),
	ENDCFGSTRUCT
};
GdkFont *fixedfont;

static GtkWidget *mainwindow;
static GtkWidget *textbox;

static int guiexit=0;

gint delete_event( GtkWidget *widget,
                   GdkEvent  *event,
		   gpointer   data )
{
    guiexit=1;
    CloseGame();
    return(FALSE);
}

static void gui_fileclose(GtkWidget *w, gpointer data)
{
 CloseGame();
}

static void gui_nesreset()
{
 FCEUI_ResetNES();
}

static void gui_nespower()
{
 FCEUI_PowerNES();
}

static void gui_quit(GtkWidget *w, gpointer data)
{
 gtk_widget_destroy(mainwindow);
 delete_event(0,0,0);
}
/* This is the GtkItemFactoryEntry structure used to generate new menus.
   Item 1: The menu path. The letter after the underscore indicates an
           accelerator key once the menu is open.
   Item 2: The accelerator key for the entry
   Item 3: The callback function.
   Item 4: The callback action.  This changes the parameters with
           which the function is called.  The default is 0.
   Item 5: The item type, used to define what kind of an item it is.
           Here are the possible values:

           NULL               -> "<Item>"
           ""                 -> "<Item>"
           "<Title>"          -> create a title item
           "<Item>"           -> create a simple item
           "<CheckItem>"      -> create a check item
           "<ToggleItem>"     -> create a toggle item
           "<RadioItem>"      -> create a radio item
           <path>             -> path of a radio item to link against
           "<Separator>"      -> create a separator
           "<Branch>"         -> create an item to hold sub items (optional)
           "<LastBranch>"     -> create a right justified branch 
*/

static GtkItemFactoryEntry menu_items[] = {
  { "/_File",         NULL,         NULL, 0, "<Branch>" },
  { "/File/_Open...",    "<control>O", gui_fileopen, 0, NULL },
  { "/File/_Close...",   "<control>C", gui_fileclose, 0, NULL },
  { "/File/sep1",     NULL,         NULL, 0, "<Separator>" },
  { "/File/Quit",     "<control>Q", gui_quit, 0, NULL },
  { "/_NES",		NULL,		NULL, 0, "<Branch>" },
  { "/NES/_Reset",	NULL,		gui_nesreset,	0,	NULL},
  { "/NES/_Power",	NULL,		gui_nespower,	0,	NULL},
  { "/NES/sep1",	NULL,	NULL,	0,	"<Separator>"	},
  { "/NES/Cheat _Search...",	NULL,	NULL,	0,	NULL	},
  { "/NES/_Cheats...",	NULL,	NULL,	0,	NULL	},
  { "/_Configuration",      NULL,         NULL, 0, "<Branch>" },
  { "/Configuration/_Sound...",  NULL,	NULL,	0,	NULL	},
  { "/Configuration/_Video...",  NULL,         gui_videoconfig, 0, NULL },
  { "/_Help",         NULL,         NULL, 0, "<LastBranch>" },
  { "/_Help/_About...",   NULL,         NULL, 0, NULL },
};


void get_main_menu( GtkWidget  *window,
                    GtkWidget **menubar )
{
  GtkItemFactory *item_factory;
  GtkAccelGroup *accel_group;
  gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

  accel_group = gtk_accel_group_new ();

  /* This function initializes the item factory.
     Param 1: The type of menu - can be GTK_TYPE_MENU_BAR, GTK_TYPE_MENU,
              or GTK_TYPE_OPTION_MENU.
     Param 2: The path of the menu.
     Param 3: A pointer to a gtk_accel_group.  The item factory sets up
              the accelerator table while generating menus.
  */

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", 
				       accel_group);

  /* This function generates the menu items. Pass the item factory,
     the number of items in the array, the array itself, and any
     callback data for the the menu items. */
  gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);

  /* Attach the new accelerator group to the window. */
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  if (menubar)
    /* Finally, return the actual menu bar created by the item factory. */ 
    *menubar = gtk_item_factory_get_widget (item_factory, "<main>");
}

void GUI_TextOut(char *string)
{
 gtk_text_insert(textbox,fixedfont,NULL,NULL,string,-1);
}

void GUI_Init(int *argc, char **argv[])
{
	GtkWidget *mainvbox;
	GtkWidget *menubar,*sep;
	GtkWidget *textscroll;
	gtk_init(argc,argv);

	if(!(fixedfont=gdk_font_load("8x16")))
	 if(!(fixedfont=gdk_font_load("fixed")))
	 printf("Ack\n\n");

	mainwindow=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(mainwindow), "delete_event",
				GTK_SIGNAL_FUNC(delete_event),NULL);
	gtk_container_set_border_width(GTK_CONTAINER(mainwindow),1);
	gtk_window_set_title(GTK_WINDOW(mainwindow),"FCE Ultra");

	mainvbox=gtk_vbox_new(FALSE,2);
	gtk_container_border_width(GTK_CONTAINER(mainvbox), 1);
	gtk_container_add(GTK_CONTAINER(mainwindow), mainvbox);

	get_main_menu(mainwindow,&menubar);

	gtk_box_pack_start(GTK_BOX(mainvbox),menubar,FALSE,TRUE,0);

	textbox=gtk_text_new(NULL,NULL);
	gtk_text_set_editable(GTK_TEXT(textbox),FALSE);

	textscroll=gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(textscroll),GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(mainvbox),textscroll,TRUE,TRUE,0);
	gtk_container_add(GTK_CONTAINER(textscroll),textbox);

	gtk_widget_show(textscroll);
        gtk_widget_show(textbox);
	gtk_widget_show(mainvbox);
	gtk_widget_show(menubar);
	gtk_widget_show(mainwindow);
}

void GUI_Update(void)
{
  while (gtk_events_pending())
                gtk_main_iteration();
}

int GUI_Idle(void)
{
  while (gtk_events_pending())
                gtk_main_iteration();
  usleep(10000);
  return(!guiexit);
}
