/*  phone
 *  Copyright (C) 1999 P.J. Caulfield
 *
 *  PJC: Quite a lot of this code was generated by GLADE, a GUI builder
 *       for GTK+
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdnet/dnetdb.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "gtkphonesig.h"
#include "gtkphonesrc.h"

GtkWidget*
get_widget                             (GtkWidget       *widget,
                                        gchar           *widget_name)
{
  GtkWidget *parent, *found_widget;

  for (;;)
    {
      if (GTK_IS_MENU (widget))
        parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
      else
        parent = widget->parent;
      if (parent == NULL)
        break;
      widget = parent;
    }

  found_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (widget),
                                                   widget_name);
  if (!found_widget)
    g_warning ("Widget not found: %s", widget_name);
  return found_widget;
}

/* This is an internally used function to set notebook tab widgets. */
void
set_notebook_tab                       (GtkWidget       *notebook,
                                        gint             page_num,
                                        GtkWidget       *widget)
{
  GtkNotebookPage *page;
  GtkWidget *notebook_page;

  page = (GtkNotebookPage*) g_list_nth (GTK_NOTEBOOK (notebook)->children, page_num)->data;
  notebook_page = page->child;
  gtk_widget_ref (notebook_page);
  gtk_notebook_remove_page (GTK_NOTEBOOK (notebook), page_num);
  gtk_notebook_insert_page (GTK_NOTEBOOK (notebook), notebook_page,
                            widget, page_num);
  gtk_widget_unref (notebook_page);
}

static GList *pixmaps_directories = NULL;

/* Use this function to set the directory containing installed pixmaps. */
void
add_pixmap_directory                   (gchar           *directory)
{
  pixmaps_directories = g_list_prepend (pixmaps_directories, g_strdup (directory));
}

/* This is an internally used function to check if a pixmap file exists. */
#ifndef G_DIR_SEPARATOR_S
#define G_DIR_SEPARATOR_S "/"
#endif
gchar*
check_file_exists                      (gchar           *directory,
                                        gchar           *filename)
{
  gchar *full_filename;
  struct stat s;
  gint status;

  full_filename = g_malloc (strlen (directory) + 1 + strlen (filename) + 1);
  strcpy (full_filename, directory);
  strcat (full_filename, G_DIR_SEPARATOR_S);
  strcat (full_filename, filename);

  status = stat (full_filename, &s);
  if (status == 0 && S_ISREG (s.st_mode))
    return full_filename;
  g_free (full_filename);
  return NULL;
}

/* This is an internally used function to create pixmaps. */
GtkWidget*
create_pixmap                          (GtkWidget       *widget,
                                        gchar           *filename)
{
  gchar *found_filename = NULL;
  GdkColormap *colormap;
  GdkPixmap *gdkpixmap;
  GdkBitmap *mask;
  GtkWidget *pixmap;
  GList *elem;

  /* We first try any pixmaps directories set by the application. */
  elem = pixmaps_directories;
  while (elem)
    {
      found_filename = check_file_exists ((gchar*)elem->data, filename);
      if (found_filename)
        break;
      elem = elem->next;
    }

  /* If we haven't found the pixmap, try the source directory. */
  if (!found_filename)
    {
      found_filename = check_file_exists ("pixmaps", filename);
    }

  if (!found_filename)
    {
      g_print ("Couldn't find pixmap file: %s\n", filename);
      return NULL;
    }

  colormap = gtk_widget_get_colormap (widget);
  gdkpixmap = gdk_pixmap_colormap_create_from_xpm (NULL, colormap, &mask,
                                                   NULL, found_filename);
  g_free (found_filename);
  if (gdkpixmap == NULL)
    return NULL;
  pixmap = gtk_pixmap_new (gdkpixmap, mask);
  gdk_pixmap_unref (gdkpixmap);
  gdk_bitmap_unref (mask);
  return pixmap;
}

GtkWidget*
create_MainWindow ()
{
  GtkWidget *MainWindow;
  GtkWidget *vbox1;
  GtkWidget *menubar;
  GtkWidget *File;
  GtkWidget *File_menu;
  GtkWidget *Quit;
  GtkWidget *Phone;
  GtkWidget *Phone_menu;
  GtkWidget *Dial;
  GtkWidget *Answer;
  GtkWidget *Reject;
  GtkWidget *Hangup;
  GtkWidget *Hold;
  GtkWidget *Directory;
  GtkWidget *Facsimile;
  GtkWidget *Help;
  GtkWidget *Help_menu;
  GtkWidget *Commands;
  GtkWidget *About;
  GtkWidget *toolbar;
  GtkWidget *tmp_toolbar_icon;
  GtkWidget *Dial_button;
  GtkWidget *Answer_button;
  GtkWidget *Reject_button;
  GtkWidget *Hold_button;
  GtkWidget *Directory_button;
  GtkWidget *Facsimile_button;
  GtkWidget *Hangup_button;
  GtkWidget *Help_button;
  GtkWidget *Exit_button;
  GtkWidget *LocalTitle;
  GtkWidget *hbox1;
  GtkWidget *talkbox;
  GtkWidget *LocalText;
  GtkWidget *vscrollbar;
  GtkWidget *statusbar;
  GtkAccelGroup *accel_group;

  MainWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (MainWindow, "MainWindow");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "MainWindow", MainWindow);
  gtk_window_set_title (GTK_WINDOW (MainWindow), "Phone");
  gtk_window_set_policy (GTK_WINDOW (MainWindow), TRUE, TRUE, FALSE);
  gtk_signal_connect (GTK_OBJECT (MainWindow), "destroy",
                      GTK_SIGNAL_FUNC (on_Quit_activate),
                      NULL);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox1, "vbox1");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "vbox1", vbox1);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (MainWindow), vbox1);

  menubar = gtk_menu_bar_new ();
  gtk_widget_set_name (menubar, "menubar");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "menubar", menubar);
  gtk_widget_show (menubar);
  gtk_box_pack_start (GTK_BOX (vbox1), menubar, FALSE, TRUE, 0);

  File = gtk_menu_item_new_with_label ("File");
  gtk_widget_set_name (File, "File");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "File", File);
  gtk_widget_show (File);
  gtk_container_add (GTK_CONTAINER (menubar), File);

  File_menu = gtk_menu_new ();
  gtk_widget_set_name (File_menu, "File_menu");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "File_menu", File_menu);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (File), File_menu);

  Quit = gtk_menu_item_new_with_label ("Quit");
  gtk_widget_set_name (Quit, "Quit");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Quit", Quit);
  gtk_widget_show (Quit);
  gtk_container_add (GTK_CONTAINER (File_menu), Quit);
  gtk_signal_connect (GTK_OBJECT (Quit), "activate",
                      GTK_SIGNAL_FUNC (on_Quit_activate),
                      NULL);
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (MainWindow), accel_group);
  gtk_widget_add_accelerator (Quit, "activate", accel_group,
                              GDK_Q, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

  Phone = gtk_menu_item_new_with_label ("Phone");
  gtk_widget_set_name (Phone, "Phone");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Phone", Phone);
  gtk_widget_show (Phone);
  gtk_container_add (GTK_CONTAINER (menubar), Phone);

  Phone_menu = gtk_menu_new ();
  gtk_widget_set_name (Phone_menu, "Phone_menu");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Phone_menu", Phone_menu);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (Phone), Phone_menu);

  Dial = gtk_menu_item_new_with_label ("Dial");
  gtk_widget_set_name (Dial, "Dial");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Dial", Dial);
  gtk_widget_show (Dial);
  gtk_container_add (GTK_CONTAINER (Phone_menu), Dial);
  gtk_signal_connect (GTK_OBJECT (Dial), "activate",
                      GTK_SIGNAL_FUNC (on_Dial_activate),
                      NULL);
  gtk_widget_add_accelerator (Dial, "activate", accel_group,
                              GDK_D, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

  Answer = gtk_menu_item_new_with_label ("Answer");
  gtk_widget_set_name (Answer, "Answer");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Answer", Answer);
  gtk_widget_show (Answer);
  gtk_container_add (GTK_CONTAINER (Phone_menu), Answer);
  gtk_signal_connect (GTK_OBJECT (Answer), "activate",
                      GTK_SIGNAL_FUNC (on_Answer_activate),
                      NULL);
  gtk_widget_add_accelerator (Answer, "activate", accel_group,
                              GDK_A, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

  Reject = gtk_menu_item_new_with_label ("Reject");
  gtk_widget_set_name (Reject, "Reject");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Reject", Reject);
  gtk_widget_show (Reject);
  gtk_container_add (GTK_CONTAINER (Phone_menu), Reject);
  gtk_signal_connect (GTK_OBJECT (Reject), "activate",
                      GTK_SIGNAL_FUNC (on_Reject_activate),
                      NULL);
  gtk_widget_add_accelerator (Reject, "activate", accel_group,
                              GDK_J, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

  Hold = gtk_menu_item_new_with_label ("Hold");
  gtk_widget_set_name (Hold, "Hold");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Hold", Hold);
  gtk_widget_show (Hold);
  gtk_container_add (GTK_CONTAINER (Phone_menu), Hold);
  gtk_signal_connect (GTK_OBJECT (Hold), "activate",
                      GTK_SIGNAL_FUNC (on_Hold_activate),
                      NULL);
  gtk_widget_add_accelerator (Hold, "activate", accel_group,
                              GDK_H, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

  Hangup = gtk_menu_item_new_with_label ("Hangup");
  gtk_widget_set_name (Hangup, "Hangup");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Hangup", Hold);
  gtk_widget_show (Hangup);
  gtk_container_add (GTK_CONTAINER (Phone_menu), Hangup);
  gtk_signal_connect (GTK_OBJECT (Hangup), "activate",
                      GTK_SIGNAL_FUNC (on_Hangup_activate),
                      NULL);
  gtk_widget_add_accelerator (Hangup, "activate", accel_group,
                              GDK_G, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

  Directory = gtk_menu_item_new_with_label ("Directory");
  gtk_widget_set_name (Directory, "Directory");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Directory", Directory);
  gtk_widget_show (Directory);
  gtk_container_add (GTK_CONTAINER (Phone_menu), Directory);
  gtk_signal_connect (GTK_OBJECT (Directory), "activate",
                      GTK_SIGNAL_FUNC (on_Directory_activate),
                      NULL);
  gtk_widget_add_accelerator (Directory, "activate", accel_group,
                              GDK_R, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

  Facsimile = gtk_menu_item_new_with_label ("Facsimile");
  gtk_widget_set_name (Facsimile, "Facsimile");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Facsimile", Facsimile);
  gtk_widget_show (Facsimile);
  gtk_container_add (GTK_CONTAINER (Phone_menu), Facsimile);
  gtk_signal_connect (GTK_OBJECT (Facsimile), "activate",
                      GTK_SIGNAL_FUNC (on_Facsimile_activate),
                      NULL);
  gtk_widget_add_accelerator (Facsimile, "activate", accel_group,
                              GDK_F, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

  Help = gtk_menu_item_new_with_label ("Help");
  gtk_widget_set_name (Help, "Help");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Help", Help);
  gtk_widget_show (Help);
  gtk_container_add (GTK_CONTAINER (menubar), Help);
  gtk_menu_item_right_justify (GTK_MENU_ITEM (Help));

  Help_menu = gtk_menu_new ();
  gtk_widget_set_name (Help_menu, "Help_menu");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Help_menu", Help_menu);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (Help), Help_menu);

  Commands = gtk_menu_item_new_with_label ("Commands");
  gtk_widget_set_name (Commands, "Commands");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Commands", Commands);
  gtk_widget_show (Commands);
  gtk_container_add (GTK_CONTAINER (Help_menu), Commands);
  gtk_signal_connect (GTK_OBJECT (Commands), "activate",
                      GTK_SIGNAL_FUNC (on_Commands_activate),
                      NULL);

  About = gtk_menu_item_new_with_label ("About");
  gtk_widget_set_name (About, "About");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "About", About);
  gtk_widget_show (About);
  gtk_container_add (GTK_CONTAINER (Help_menu), About);
  gtk_signal_connect (GTK_OBJECT (About), "activate",
                      GTK_SIGNAL_FUNC (on_About_activate),
                      NULL);

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
  gtk_widget_set_name (toolbar, "toolbar");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "toolbar", toolbar);
  gtk_widget_show (toolbar);
  gtk_box_pack_start (GTK_BOX (vbox1), toolbar, FALSE, TRUE, 0);
  gtk_toolbar_set_tooltips(GTK_TOOLBAR(toolbar), 1);
  
  tmp_toolbar_icon = create_pixmap (MainWindow, "dial.xpm");
  Dial_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                "Dial",
                                "Dial another user", NULL,
                                tmp_toolbar_icon, NULL, NULL);
  gtk_widget_set_name (Dial_button, "Dial_button");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Dial_button", Dial_button);
  gtk_widget_show (Dial_button);
  gtk_signal_connect (GTK_OBJECT (Dial_button), "clicked",
                      GTK_SIGNAL_FUNC (on_Dial_activate),
                      NULL);
  
  tmp_toolbar_icon = create_pixmap (MainWindow, "answer.xpm");
  Answer_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                "Answer",
                                "Answer an incoming call", NULL,
                                tmp_toolbar_icon, NULL, NULL);
  gtk_widget_set_name (Answer_button, "Answer_button");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Answer_button", Answer_button);
  gtk_widget_show (Answer_button);
  gtk_signal_connect (GTK_OBJECT (Answer_button), "clicked",
                      GTK_SIGNAL_FUNC (on_Answer_activate),
                      NULL);

  tmp_toolbar_icon = create_pixmap (MainWindow, "reject.xpm");
  Reject_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                "Reject",
                                "Reject an incoming call", NULL,
                                tmp_toolbar_icon, NULL, NULL);
  gtk_widget_set_name (Reject_button, "Reject_button");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Reject_button", Reject_button);
  gtk_widget_show (Reject_button);
  gtk_signal_connect (GTK_OBJECT (Reject_button), "clicked",
                      GTK_SIGNAL_FUNC (on_Reject_activate),
                      NULL);

  tmp_toolbar_icon = create_pixmap (MainWindow, "hold.xpm");
  Hold_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                "Hold",
                                "Put all callers on hold", NULL,
                                tmp_toolbar_icon, NULL, NULL);
  gtk_widget_set_name (Hold_button, "Hold_button");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Hold_button", Hold_button);
  gtk_widget_show (Hold_button);
  gtk_signal_connect (GTK_OBJECT (Hold_button), "clicked",
                      GTK_SIGNAL_FUNC (on_Hold_activate),
                      NULL);

  tmp_toolbar_icon = create_pixmap (MainWindow, "directory.xpm");
  Directory_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
				"Directory",
				"Show a list of users logged into a node",
				NULL,
                                tmp_toolbar_icon, NULL, NULL);
  gtk_widget_set_name (Directory_button, "Directory_button");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Directory_button", Directory_button);
  gtk_widget_show (Directory_button);
  gtk_signal_connect (GTK_OBJECT (Directory_button), "clicked",
                      GTK_SIGNAL_FUNC (on_Directory_activate),
                      NULL);

  tmp_toolbar_icon = create_pixmap (MainWindow, "facsimile.xpm");
  Facsimile_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                "Facsimile",
                                "Send a file to users", NULL,
                                tmp_toolbar_icon, NULL, NULL);
  gtk_widget_set_name (Facsimile_button, "Facsimile_button");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Facsimile_button", Facsimile_button);
  gtk_widget_show (Facsimile_button);
  gtk_signal_connect (GTK_OBJECT (Facsimile_button), "clicked",
                      GTK_SIGNAL_FUNC (on_Facsimile_activate),
                      NULL);

  tmp_toolbar_icon = create_pixmap (MainWindow, "hangup.xpm");
  Hangup_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                "Hangup",
                                "Hangup the phone", NULL,
                                tmp_toolbar_icon, NULL, NULL);
  gtk_widget_set_name (Hangup_button, "Hangup_button");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Hangup_button", Hangup_button);
  gtk_widget_show (Hangup_button);
  gtk_signal_connect (GTK_OBJECT (Hangup_button), "clicked",
                      GTK_SIGNAL_FUNC (on_Hangup_activate),
                      NULL);

  tmp_toolbar_icon = create_pixmap (MainWindow, "help.xpm");
  Help_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                "Help",
                                "Show version number", NULL,
                                tmp_toolbar_icon, NULL, NULL);
  gtk_widget_set_name (Help_button, "Help_button");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Help_button", Help_button);
  gtk_widget_show (Help_button);
  gtk_signal_connect (GTK_OBJECT (Help_button), "clicked",
                      GTK_SIGNAL_FUNC (on_About_activate),
                      NULL);

  tmp_toolbar_icon = create_pixmap (MainWindow, "exit.xpm");
  Exit_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                "Exit",
                                "Hangup and exit the program", NULL,
                                tmp_toolbar_icon, NULL, NULL);
  gtk_widget_set_name (Exit_button, "Exit_button");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "Exit_button", Exit_button);
  gtk_widget_show (Exit_button);
  gtk_signal_connect (GTK_OBJECT (Exit_button), "clicked",
                      GTK_SIGNAL_FUNC (on_Quit_activate),
                      NULL);

  LocalTitle = gtk_label_new ("The great prophet Zarquon");
  gtk_widget_set_name (LocalTitle, "LocalTitle");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "LocalTitle", LocalTitle);
  gtk_widget_show (LocalTitle);
  gtk_box_pack_start (GTK_BOX (vbox1), LocalTitle, FALSE, FALSE, 0);

  talkbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (talkbox, "talkbox");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "talkbox", talkbox);
  gtk_widget_show (talkbox);
  gtk_box_pack_start (GTK_BOX (vbox1), talkbox, TRUE, TRUE, 0);

  // A box to pack the scrollbar into
  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (talkbox), hbox1, TRUE, TRUE, 0);

  LocalText = gtk_text_new (NULL, NULL);
  gtk_widget_set_name (LocalText, "LocalText");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "LocalText", LocalText);
  gtk_widget_show (LocalText);
  gtk_box_pack_start (GTK_BOX (hbox1), LocalText, TRUE, TRUE, 0);
  gtk_text_set_editable (GTK_TEXT (LocalText), FALSE);
  gtk_signal_connect_after (GTK_OBJECT (LocalText), "key_press_event",
                            GTK_SIGNAL_FUNC (on_LocalText_key_press_event),
                            NULL);

  vscrollbar = gtk_vscrollbar_new (GTK_TEXT(LocalText)->vadj);
  gtk_box_pack_start(GTK_BOX(hbox1), vscrollbar, FALSE, FALSE, 0);
  gtk_widget_show (vscrollbar);

  
  statusbar = gtk_statusbar_new ();
  gtk_widget_set_name (statusbar, "statusbar");
  gtk_object_set_data (GTK_OBJECT (MainWindow), "statusbar", statusbar);
  gtk_widget_show (statusbar);
  gtk_box_pack_start (GTK_BOX (vbox1), statusbar, FALSE, TRUE, 0);

  return MainWindow;
}


GtkWidget*
create_DialDialog (void)
{
  GtkWidget *DialDialog;
  GtkWidget *dialog_vbox1;
  GtkWidget *fixed2;
  GtkWidget *entry2;
  GtkWidget *label4;
  GtkWidget *label5;
  GtkWidget *combo4;
  GtkWidget *dialog_action_area1;
  GtkWidget *hbox1;
  GtkWidget *fixed3;
  GtkWidget *DialCancel;
  GtkWidget *DialOK;
  void      *nodelist;
  char      *nodename;
  GList     *glist=NULL;

  DialDialog = gtk_dialog_new ();
  gtk_widget_set_name (DialDialog, "DialDialog");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "DialDialog", DialDialog);
  gtk_window_set_title (GTK_WINDOW (DialDialog), "Dial User");
  gtk_window_set_policy (GTK_WINDOW (DialDialog), TRUE, TRUE, FALSE);
  gtk_widget_set_usize (DialDialog, 308, 150);
  GTK_WINDOW (DialDialog)->type = GTK_WINDOW_DIALOG;
  gtk_window_position (GTK_WINDOW (DialDialog), GTK_WIN_POS_CENTER);

  
  dialog_vbox1 = GTK_DIALOG (DialDialog)->vbox;
  gtk_widget_set_name (dialog_vbox1, "dialog_vbox1");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "dialog_vbox1", dialog_vbox1);
  gtk_widget_show (dialog_vbox1);

  fixed2 = gtk_fixed_new ();
  gtk_widget_set_name (fixed2, "fixed2");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "fixed2", fixed2);
  gtk_widget_show (fixed2);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), fixed2, TRUE, TRUE, 0);
  gtk_widget_set_usize (fixed2, -1, 60);

  entry2 = gtk_entry_new ();
  gtk_widget_set_name (entry2, "entry2");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "entry2", entry2);
  gtk_widget_show (entry2);
  gtk_fixed_put (GTK_FIXED (fixed2), entry2, 168, 56);
  gtk_widget_set_usize (entry2, 120, 24);

  label4 = gtk_label_new ("Remote Node");
  gtk_widget_set_name (label4, "label4");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "label4", label4);
  gtk_widget_show (label4);
  gtk_fixed_put (GTK_FIXED (fixed2), label4, 24, 32);
  gtk_widget_set_usize (label4, 88, 16);
  gtk_label_set_justify (GTK_LABEL (label4), GTK_JUSTIFY_LEFT);

  label5 = gtk_label_new ("Remote user");
  gtk_widget_set_name (label5, "label5");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "label5", label5);
  gtk_widget_show (label5);
  gtk_fixed_put (GTK_FIXED (fixed2), label5, 168, 32);
  gtk_widget_set_usize (label5, 104, 16);
  gtk_label_set_justify (GTK_LABEL (label5), GTK_JUSTIFY_LEFT);

  combo4 = gtk_combo_new ();
  gtk_widget_set_name (combo4, "combo4");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "combo4", combo4);
  gtk_widget_show (combo4);
  gtk_fixed_put (GTK_FIXED (fixed2), combo4, 24, 56);
  gtk_widget_set_usize (GTK_COMBO (combo4)->entry, 112, 24);
  gtk_widget_set_usize (combo4, 128, 24);

  dialog_action_area1 = GTK_DIALOG (DialDialog)->action_area;
  gtk_widget_set_name (dialog_action_area1, "dialog_action_area1");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "dialog_action_area1", dialog_action_area1);
  gtk_widget_show (dialog_action_area1);
  gtk_container_border_width (GTK_CONTAINER (dialog_action_area1), 10);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox1, "hbox1");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "hbox1", hbox1);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (dialog_action_area1), hbox1, TRUE, TRUE, 0);

  fixed3 = gtk_fixed_new ();
  gtk_widget_set_name (fixed3, "fixed3");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "fixed3", fixed3);
  gtk_widget_show (fixed3);
  gtk_box_pack_start (GTK_BOX (hbox1), fixed3, TRUE, TRUE, 0);

  DialCancel = gtk_button_new_with_label ("Cancel");
  gtk_widget_set_name (DialCancel, "DialCancel");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "DialCancel", DialCancel);
  gtk_widget_show (DialCancel);
  gtk_fixed_put (GTK_FIXED (fixed3), DialCancel, 208, 0);
  gtk_widget_set_usize (DialCancel, 80, 33);
  GTK_WIDGET_SET_FLAGS (DialCancel, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (DialCancel), "clicked",
                      GTK_SIGNAL_FUNC (on_DialCancel_activate),
                      DialDialog);

  DialOK = gtk_button_new_with_label ("OK");
  gtk_widget_set_name (DialOK, "DialOK");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "DialOK", DialOK);
  gtk_widget_show (DialOK);
  gtk_fixed_put (GTK_FIXED (fixed3), DialOK, 120, 0);
  gtk_widget_set_usize (DialOK, 80, 33);
  GTK_WIDGET_SET_FLAGS (DialOK, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (DialOK), "clicked",
                      GTK_SIGNAL_FUNC (on_DialOK_activate),
                      DialDialog);

/* Fill the dialog */
  nodelist = dnet_getnode();
  nodename = dnet_nextnode(nodelist);
  while(nodename)
  {
      char *node = malloc(strlen(nodename)+1);
      strcpy(node, nodename);
      glist = g_list_append(glist, node);
      nodename = dnet_nextnode(nodelist);
  }
  dnet_endnode(nodelist);
  gtk_combo_set_popdown_strings( GTK_COMBO(combo4), glist) ;
  
  return DialDialog;
}

GtkWidget*
create_DirDialog (void)
{
  GtkWidget *DialDialog;
  GtkWidget *dialog_vbox1;
  GtkWidget *fixed2;
  GtkWidget *entry2;
  GtkWidget *label4;
  GtkWidget *label5;
  GtkWidget *combo4;
  GtkWidget *dialog_action_area1;
  GtkWidget *hbox1;
  GtkWidget *fixed3;
  GtkWidget *DialCancel;
  GtkWidget *DialOK;
  void      *nodelist;
  char      *nodename;
  GList     *glist=NULL;

  DialDialog = gtk_dialog_new ();
  gtk_widget_set_name (DialDialog, "DialDialog");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "DialDialog", DialDialog);
  gtk_window_set_title (GTK_WINDOW (DialDialog), "Directory");
  gtk_window_set_policy (GTK_WINDOW (DialDialog), TRUE, TRUE, FALSE);
  gtk_widget_set_usize (DialDialog, 180, 150);
  GTK_WINDOW (DialDialog)->type = GTK_WINDOW_DIALOG;
  gtk_window_position (GTK_WINDOW (DialDialog), GTK_WIN_POS_CENTER);
  
  dialog_vbox1 = GTK_DIALOG (DialDialog)->vbox;
  gtk_widget_set_name (dialog_vbox1, "dialog_vbox1");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "dialog_vbox1", dialog_vbox1);
  gtk_widget_show (dialog_vbox1);

  fixed2 = gtk_fixed_new ();
  gtk_widget_set_name (fixed2, "fixed2");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "fixed2", fixed2);
  gtk_widget_show (fixed2);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), fixed2, TRUE, TRUE, 0);
  gtk_widget_set_usize (fixed2, -1, 60);

  label4 = gtk_label_new ("Remote Node");
  gtk_widget_set_name (label4, "label4");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "label4", label4);
  gtk_widget_show (label4);
  gtk_fixed_put (GTK_FIXED (fixed2), label4, 24, 32);
  gtk_widget_set_usize (label4, 88, 16);
  gtk_label_set_justify (GTK_LABEL (label4), GTK_JUSTIFY_LEFT);

  combo4 = gtk_combo_new ();
  gtk_widget_set_name (combo4, "combo4");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "combo4", combo4);
  gtk_widget_show (combo4);
  gtk_fixed_put (GTK_FIXED (fixed2), combo4, 24, 56);
  gtk_widget_set_usize (GTK_COMBO (combo4)->entry, 112, 24);
  gtk_widget_set_usize (combo4, 128, 24);

  dialog_action_area1 = GTK_DIALOG (DialDialog)->action_area;
  gtk_widget_set_name (dialog_action_area1, "dialog_action_area1");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "dialog_action_area1", dialog_action_area1);
  gtk_widget_show (dialog_action_area1);
  gtk_container_border_width (GTK_CONTAINER (dialog_action_area1), 10);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox1, "hbox1");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "hbox1", hbox1);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (dialog_action_area1), hbox1, TRUE, TRUE, 0);

  fixed3 = gtk_fixed_new ();
  gtk_widget_set_name (fixed3, "fixed3");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "fixed3", fixed3);
  gtk_widget_show (fixed3);
  gtk_box_pack_start (GTK_BOX (hbox1), fixed3, TRUE, TRUE, 0);

  DialCancel = gtk_button_new_with_label ("Cancel");
  gtk_widget_set_name (DialCancel, "DialCancel");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "DialCancel", DialCancel);
  gtk_widget_show (DialCancel);
  gtk_fixed_put (GTK_FIXED (fixed3), DialCancel, 80, 0);
  gtk_widget_set_usize (DialCancel, 80, 33);
  GTK_WIDGET_SET_FLAGS (DialCancel, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (DialCancel), "clicked",
                      GTK_SIGNAL_FUNC (on_DialCancel_activate),
                      DialDialog);

  DialOK = gtk_button_new_with_label ("OK");
  gtk_widget_set_name (DialOK, "DialOK");
  gtk_object_set_data (GTK_OBJECT (DialDialog), "DialOK", DialOK);
  gtk_widget_show (DialOK);
  gtk_fixed_put (GTK_FIXED (fixed3), DialOK, 0, 0);
  gtk_widget_set_usize (DialOK, 80, 33);
  GTK_WIDGET_SET_FLAGS (DialOK, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (DialOK), "clicked",
                      GTK_SIGNAL_FUNC (on_DirOK_activate),
                      DialDialog);

/* Fill the dialog */
  nodelist = dnet_getnode();
  nodename = dnet_nextnode(nodelist);
  while(nodename)
  {
      char *node = malloc(strlen(nodename)+1);
      strcpy(node, nodename);
      glist = g_list_append(glist, node);
      nodename = dnet_nextnode(nodelist);
  }
  dnet_endnode(nodelist);
  gtk_combo_set_popdown_strings( GTK_COMBO(combo4), glist) ;
  
  return DialDialog;
}

GtkWidget*
create_FacsimileDialog (void)
{
  GtkWidget *FacsimileDialog;
  GtkWidget *ok_button1;
  GtkWidget *cancel_button1;

  FacsimileDialog = gtk_file_selection_new ("Select file to send");
  gtk_widget_set_name (FacsimileDialog, "FacsimileDialog");
  gtk_object_set_data (GTK_OBJECT (FacsimileDialog), "FacsimileDialog", FacsimileDialog);
  gtk_container_border_width (GTK_CONTAINER (FacsimileDialog), 10);
  GTK_WINDOW (FacsimileDialog)->type = GTK_WINDOW_DIALOG;
  gtk_window_position (GTK_WINDOW (FacsimileDialog), GTK_WIN_POS_CENTER);

  ok_button1 = GTK_FILE_SELECTION (FacsimileDialog)->ok_button;
  gtk_widget_set_name (ok_button1, "ok_button1");
  gtk_object_set_data (GTK_OBJECT (FacsimileDialog), "ok_button1", ok_button1);
  gtk_widget_show (ok_button1);
  GTK_WIDGET_SET_FLAGS (ok_button1, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (ok_button1), "clicked",
                      GTK_SIGNAL_FUNC (on_FacsimileOK_activate),
                      FacsimileDialog);


  
  cancel_button1 = GTK_FILE_SELECTION (FacsimileDialog)->cancel_button;
  gtk_widget_set_name (cancel_button1, "cancel_button1");
  gtk_object_set_data (GTK_OBJECT (FacsimileDialog), "cancel_button1", cancel_button1);
  gtk_widget_show (cancel_button1);
  GTK_WIDGET_SET_FLAGS (cancel_button1, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (cancel_button1), "clicked",
                      GTK_SIGNAL_FUNC (on_DialCancel_activate),
                      FacsimileDialog);


  return FacsimileDialog;
}


GtkWidget*
create_AboutDialog ()
{
  GtkWidget *AboutDialog;
  GtkWidget *dialog_vbox2;
  GtkWidget *vbox2;
  GtkWidget *PhonePixmap;
  GtkWidget *AboutLabel;
  GtkWidget *dialog_action_area2;
  GtkWidget *AboutOK;

  AboutDialog = gtk_dialog_new ();
  gtk_widget_set_name (AboutDialog, "AboutDialog");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "AboutDialog", AboutDialog);
  gtk_window_set_title (GTK_WINDOW (AboutDialog), "About Phone");
  gtk_window_set_policy (GTK_WINDOW (AboutDialog), TRUE, TRUE, FALSE);
  GTK_WINDOW (AboutDialog)->type = GTK_WINDOW_DIALOG;
  gtk_window_position (GTK_WINDOW (AboutDialog), GTK_WIN_POS_CENTER);
  
  dialog_vbox2 = GTK_DIALOG (AboutDialog)->vbox;
  gtk_widget_set_name (dialog_vbox2, "dialog_vbox2");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "dialog_vbox2", dialog_vbox2);
  gtk_widget_show (dialog_vbox2);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox2, "vbox2");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "vbox2", vbox2);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (dialog_vbox2), vbox2, TRUE, TRUE, 0);

  PhonePixmap = create_pixmap (AboutDialog, "phone.xpm");
  if (PhonePixmap != NULL)
  {
      gtk_widget_set_name (PhonePixmap, "PhonePixmap");
      gtk_object_set_data (GTK_OBJECT (AboutDialog), "PhonePixmap", PhonePixmap);
      gtk_widget_show (PhonePixmap);
      gtk_box_pack_start (GTK_BOX (vbox2), PhonePixmap, TRUE, FALSE, 10);
  }
  
  AboutLabel = gtk_label_new ("Phone V" VERSION " by Patrick Caulfield");
  gtk_widget_set_name (AboutLabel, "AboutLabel");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "AboutLabel", AboutLabel);
  gtk_widget_show (AboutLabel);
  gtk_box_pack_start (GTK_BOX (vbox2), AboutLabel, TRUE, TRUE, 10);

  dialog_action_area2 = GTK_DIALOG (AboutDialog)->action_area;
  gtk_widget_set_name (dialog_action_area2, "dialog_action_area2");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "dialog_action_area2", dialog_action_area2);
  gtk_widget_show (dialog_action_area2);
  gtk_container_border_width (GTK_CONTAINER (dialog_action_area2), 10);

  AboutOK = gtk_button_new_with_label ("OK");
  gtk_widget_set_name (AboutOK, "AboutOK");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "AboutOK", AboutOK);
  gtk_widget_show (AboutOK);
  gtk_box_pack_start (GTK_BOX (dialog_action_area2), AboutOK, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (AboutOK), "clicked",
                      GTK_SIGNAL_FUNC (on_DialCancel_activate),
                      AboutDialog);


  return AboutDialog;
}

GtkWidget*
create_DisplayDialog (char *name)
{
  GtkWidget *AboutDialog;
  GtkWidget *dialog_vbox2;
  GtkWidget *vbox2;
  GtkWidget *AboutLabel;
  GtkWidget *dialog_action_area2;
  GtkWidget *AboutOK;

  AboutDialog = gtk_dialog_new ();
  gtk_widget_set_name (AboutDialog, "AboutDialog");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "AboutDialog", AboutDialog);
  gtk_window_set_title (GTK_WINDOW (AboutDialog), name);
  gtk_window_set_policy (GTK_WINDOW (AboutDialog), TRUE, TRUE, FALSE);
  GTK_WINDOW (AboutDialog)->type = GTK_WINDOW_DIALOG;
  gtk_window_position (GTK_WINDOW (AboutDialog), GTK_WIN_POS_CENTER);

  dialog_vbox2 = GTK_DIALOG (AboutDialog)->vbox;
  gtk_widget_set_name (dialog_vbox2, "dialog_vbox2");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "dialog_vbox2", dialog_vbox2);
  gtk_widget_show (dialog_vbox2);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox2, "vbox2");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "vbox2", vbox2);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (dialog_vbox2), vbox2, FALSE, FALSE, 0);

// A blank line to start.
  AboutLabel = gtk_label_new ("");
  gtk_widget_set_name (AboutLabel, "AboutLabel");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "AboutLabel", AboutLabel);
  gtk_widget_show (AboutLabel);
  gtk_box_pack_start (GTK_BOX (vbox2), AboutLabel, FALSE, FALSE, 0);

  dialog_action_area2 = GTK_DIALOG (AboutDialog)->action_area;
  gtk_widget_set_name (dialog_action_area2, "dialog_action_area2");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "dialog_action_area2", dialog_action_area2);
  gtk_widget_show (dialog_action_area2);
  gtk_container_border_width (GTK_CONTAINER (dialog_action_area2), 10);

  AboutOK = gtk_button_new_with_label ("OK");
  gtk_widget_set_name (AboutOK, "AboutOK");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "AboutOK", AboutOK);
  gtk_widget_show (AboutOK);
  gtk_box_pack_start (GTK_BOX (dialog_action_area2), AboutOK, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (AboutOK), "clicked",
                      GTK_SIGNAL_FUNC (on_DisplayOK_activate),
                      AboutDialog);


  return AboutDialog;
}

GtkWidget*
create_MessageDialog (void)
{
  GtkWidget *AboutDialog;
  GtkWidget *dialog_vbox2;
  GtkWidget *vbox2;
  GtkWidget *AboutLabel;
  GtkWidget *dialog_action_area2;
  GtkWidget *AboutOK;

  AboutDialog = gtk_dialog_new ();
  gtk_widget_set_name (AboutDialog, "AboutDialog");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "AboutDialog", AboutDialog);
  gtk_window_set_title (GTK_WINDOW (AboutDialog), "Phone");
  gtk_window_set_policy (GTK_WINDOW (AboutDialog), TRUE, TRUE, FALSE);
  GTK_WINDOW (AboutDialog)->type = GTK_WINDOW_DIALOG;
  gtk_window_position (GTK_WINDOW (AboutDialog), GTK_WIN_POS_CENTER);

  dialog_vbox2 = GTK_DIALOG (AboutDialog)->vbox;
  gtk_widget_set_name (dialog_vbox2, "dialog_vbox2");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "dialog_vbox2", dialog_vbox2);
  gtk_widget_show (dialog_vbox2);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox2, "vbox2");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "vbox2", vbox2);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (dialog_vbox2), vbox2, FALSE, FALSE, 0);

  dialog_action_area2 = GTK_DIALOG (AboutDialog)->action_area;
  gtk_widget_set_name (dialog_action_area2, "dialog_action_area2");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "dialog_action_area2", dialog_action_area2);
  gtk_widget_show (dialog_action_area2);
  gtk_container_border_width (GTK_CONTAINER (dialog_action_area2), 10);

  AboutOK = gtk_button_new_with_label ("OK");
  gtk_widget_set_name (AboutOK, "AboutOK");
  gtk_object_set_data (GTK_OBJECT (AboutDialog), "AboutOK", AboutOK);
  gtk_widget_show (AboutOK);
  gtk_box_pack_start (GTK_BOX (dialog_action_area2), AboutOK, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (AboutOK), "clicked",
                      GTK_SIGNAL_FUNC (on_DisplayOK_activate),
                      AboutDialog);


  return AboutDialog;
}