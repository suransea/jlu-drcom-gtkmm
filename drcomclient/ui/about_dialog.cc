//
// Created by sea on 12/23/18.
//

#include "about_dialog.h"

namespace DrcomClient {

AboutDialog::AboutDialog(BaseObjectType *object,
                         const Glib::RefPtr<Gtk::Builder> &builder)
    : Gtk::Dialog(object) {}

} // namespace DrcomClient