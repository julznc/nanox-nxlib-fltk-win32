//
// "$Id: Fl_Scrollbar.cxx 8864 2011-07-19 04:49:30Z greg.ercolano $"
//
// Scroll bar widget for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     http://www.fltk.org/COPYING.php
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

#include <config.h>
#include <FL/Fl.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/fl_draw.H>
#include <math.h>
#include "flstring.h"

#define INITIALREPEAT .5
#define REPEAT .05

void Fl_Scrollbar::increment_cb() {
  char inv = maximum()<minimum();
  int ls = inv ? -linesize_ : linesize_;
  int i;
  switch (pushed_) {
    case 1: // clicked on arrow left
      i = -ls;
      break;
    default: // clicked on arrow right
      i =  ls;
      break;
    case 5: // clicked into the box next to the slider on the left
      i = -(int((maximum()-minimum())*slider_size()/(1.0-slider_size())));
      if (inv) {
        if (i<-ls) i = -ls;
      } else {
        if (i>-ls) i = -ls; // err
      }
      break;
    case 6: // clicked into the box next to the slider on the right
      i = (int((maximum()-minimum())*slider_size()/(1.0-slider_size())));
      if (inv) {
        if (i>ls) i = ls;
      } else {
        if (i<ls) i = ls; // err
      }
      break;
  }
  handle_drag(clamp(value() + i));
}

void Fl_Scrollbar::timeout_cb(void* v) {
  Fl_Scrollbar* s = (Fl_Scrollbar*)v;
  s->increment_cb();
  Fl::add_timeout(REPEAT, timeout_cb, s);
}

int Fl_Scrollbar::handle(int event) {
  // area of scrollbar:
  int area;
  int X=x(); int Y=y(); int W=w(); int H=h();

  // adjust slider area to be inside the arrow buttons:
  if (horizontal()) {
    if (W >= 3*H) {X += H; W -= 2*H;}
  } else {
    if (H >= 3*W) {Y += W; H -= 2*W;}
  }

  // which widget part is highlighted?
  int relx;
  int ww;
  if (horizontal()) {
    relx = Fl::event_x()-X;
    ww = W;
  } else {
    relx = Fl::event_y()-Y;
    ww = H;
  }
  if (relx < 0) area = 1;
  else if (relx >= ww) area = 2;
  else {
    int S = int(slider_size()*ww+.5);
    int T = (horizontal() ? H : W)/2+1;
    if (type()==FL_VERT_NICE_SLIDER || type()==FL_HOR_NICE_SLIDER) T += 4;
    if (S < T) S = T;
    double val =
      (maximum()-minimum()) ? (value()-minimum())/(maximum()-minimum()) : 0.5;
    int sliderx;
    if (val >= 1.0) sliderx = ww-S;
    else if (val <= 0.0) sliderx = 0;
    else sliderx = int(val*(ww-S)+.5);
    if (Fl::event_button() == FL_MIDDLE_MOUSE) area = 8;
    else if (relx < sliderx) area = 5;
    else if (relx >= sliderx+S) area = 6;
    else area = 8;
  }

  switch (event) {
  case FL_ENTER:
  case FL_LEAVE:
    return 1;
  case FL_RELEASE:
      damage(FL_DAMAGE_ALL);
    if (pushed_) {
      Fl::remove_timeout(timeout_cb, this);
      pushed_ = 0;
    }
    handle_release();
    return 1;
  case FL_PUSH:
    if (pushed_) return 1;
    if (area != 8) pushed_ = area;
    if (pushed_) {
      handle_push();
      Fl::add_timeout(INITIALREPEAT, timeout_cb, this);
      increment_cb();
      damage(FL_DAMAGE_ALL);
      return 1;
    }
    return Fl_Slider::handle(event, X,Y,W,H);
  case FL_DRAG:
    if (pushed_) return 1;
    return Fl_Slider::handle(event, X,Y,W,H);
  case FL_MOUSEWHEEL :
    if (horizontal()) {
      if (Fl::e_dx==0) return 0;
      int ls = maximum()>=minimum() ? linesize_ : -linesize_;
      handle_drag(clamp(value() + ls * Fl::e_dx));
      return 1;
    } else {
      if (Fl::e_dy==0) return 0;
      int ls = maximum()>=minimum() ? linesize_ : -linesize_;
      handle_drag(clamp(value() + ls * Fl::e_dy));
      return 1;
    }
  case FL_SHORTCUT:
  case FL_KEYBOARD: {
    int v = value();
    int ls = maximum()>=minimum() ? linesize_ : -linesize_;
    if (horizontal()) {
      switch (Fl::event_key()) {
      case FL_Left:
	v -= ls;
	break;
      case FL_Right:
	v += ls;
	break;
      default:
	return 0;
      }
    } else { // vertical
      switch (Fl::event_key()) {
      case FL_Up:
	v -= ls;
	break;
      case FL_Down:
	v += ls;
	break;
      case FL_Page_Up:
	if (slider_size() >= 1.0) return 0;
	v -= int((maximum()-minimum())*slider_size()/(1.0-slider_size()));
	v += ls;
	break;
      case FL_Page_Down:
	if (slider_size() >= 1.0) return 0;
	v += int((maximum()-minimum())*slider_size()/(1.0-slider_size()));
	v -= ls;
	break;
      case FL_Home:
	v = int(minimum());
	break;
      case FL_End:
	v = int(maximum());
	break;
      default:
	return 0;
      }
    }
    v = int(clamp(v));
    if (v != value()) {
      Fl_Slider::value(v);
      value_damage();
      set_changed();
      do_callback();
    }
    return 1;}
  }
  return 0;
}

void Fl_Scrollbar::draw() {
  if (damage()&FL_DAMAGE_ALL) draw_box();
  int X = x()+Fl::box_dx(box());
  int Y = y()+Fl::box_dy(box());
  int W = w()-Fl::box_dw(box());
  int H = h()-Fl::box_dh(box());

#ifdef USE_NANOX // is small size, draw simple scrollbar
  int T = (horizontal()? H : W)/2+1;
  if( T < 4 ) {
    X = x(); Y = y(); W = w(); H = h();
    double val;
    if (minimum() == maximum())
      val = 0.5;
    else {
      val = (value()-minimum())/(maximum()-minimum());
      if (val > 1.0) val = 1.0;
      else if (val < 0.0) val = 0.0;
    }

    int ww = (horizontal()? W : H);
    int S = int(slider_size()*ww+.5);
    if (S < T) S = T;
    int xx = int(val*(ww-S)+.5);
    
    int xsl, ysl, wsl, hsl;
    if (horizontal()) {
      xsl = X+xx; wsl = S; ysl = Y; hsl = H;
    } else {
      ysl = Y+xx; hsl = S; xsl = X; wsl = W;
    }
    
    fl_push_clip(X, Y, W, H);
    if(fl_display && fl_visual->depth == 1){ // monochrome
      draw_box();
      if (wsl>0 && hsl>0) draw_box(FL_UP_BOX, xsl, ysl, wsl, hsl, selection_color());
    }else {
      draw_box(FL_FLAT_BOX, X, Y, W, H, FL_GRAY0);
      if (wsl>0 && hsl>0) draw_box(FL_FLAT_BOX, xsl, ysl, wsl, hsl, selection_color());
    }
    fl_pop_clip();
    
    return;
  }
#endif

  if (horizontal()) {
    if (W < 3*H) {Fl_Slider::draw(X,Y,W,H); return;}
    Fl_Slider::draw(X+H,Y,W-2*H,H);
    if (damage()&FL_DAMAGE_ALL) {
      draw_box((pushed_==1) ? fl_down(slider()) : slider(),
	       X, Y, H, H, selection_color());
      draw_box((pushed_==2) ? fl_down(slider()) : slider(),
	       X+W-H, Y, H, H, selection_color());
      if (active_r())
        fl_color(labelcolor());
      else
        fl_color(fl_inactive(labelcolor()));
      int w1 = (H-4)/3; if (w1 < 1) w1 = 1;
      int x1 = X+(H-w1-1)/2;
      int yy1 = Y+(H-2*w1-1)/2;
      if (Fl::scheme_ && !strcmp(Fl::scheme_, "gtk+")) {
	fl_polygon(x1, yy1+w1, x1+w1, yy1+2*w1, x1+w1-1, yy1+w1, x1+w1, yy1);
	x1 += (W-H);
	fl_polygon(x1, yy1, x1+1, yy1+w1, x1, yy1+2*w1, x1+w1, yy1+w1);
      } else {
	fl_polygon(x1, yy1+w1, x1+w1, yy1+2*w1, x1+w1, yy1);
	x1 += (W-H);
	fl_polygon(x1, yy1, x1, yy1+2*w1, x1+w1, yy1+w1);
      }
    }
  } else { // vertical
    if (H < 3*W) {Fl_Slider::draw(X,Y,W,H); return;}
    Fl_Slider::draw(X,Y+W,W,H-2*W);
    if (damage()&FL_DAMAGE_ALL) {
      draw_box((pushed_==1) ? fl_down(slider()) : slider(),
	       X, Y, W, W, selection_color());
      draw_box((pushed_==2) ? fl_down(slider()) : slider(),
	       X, Y+H-W, W, W, selection_color());
      if (active_r())
        fl_color(labelcolor());
      else
        fl_color(fl_inactive(labelcolor()));
      int w1 = (W-4)/3; if (w1 < 1) w1 = 1;
      int x1 = X+(W-2*w1-1)/2;
      int yy1 = Y+(W-w1-1)/2;
      if (Fl::scheme_ && !strcmp(Fl::scheme_, "gtk+")) {
	fl_polygon(x1, yy1+w1, x1+w1, yy1+w1-1, x1+2*w1, yy1+w1, x1+w1, yy1);
	yy1 += H-W;
	fl_polygon(x1, yy1, x1+w1, yy1+1, x1+w1, yy1+w1);
	fl_polygon(x1+w1, yy1+1, x1+2*w1, yy1, x1+w1, yy1+w1);
      } else {
	fl_polygon(x1, yy1+w1, x1+2*w1, yy1+w1, x1+w1, yy1);
	yy1 += H-W;
	fl_polygon(x1, yy1, x1+w1, yy1+w1, x1+2*w1, yy1);
      }
    }
  }
}

/**
  Creates a new Fl_Scrollbar widget with given position, size, and label.
  You need to do type(FL_HORIZONTAL) if you want a horizontal scrollbar.
*/
Fl_Scrollbar::Fl_Scrollbar(int X, int Y, int W, int H, const char* L)
  : Fl_Slider(X, Y, W, H, L) {
  box(FL_FLAT_BOX);
  color(FL_DARK2);
  slider(FL_UP_BOX);
  linesize_ = 16;
  pushed_ = 0;
  step(1);
}

/**  Destroys the Scrollbar. */
Fl_Scrollbar::~Fl_Scrollbar() {
  if (pushed_)
    Fl::remove_timeout(timeout_cb, this);
}


//
// End of "$Id: Fl_Scrollbar.cxx 8864 2011-07-19 04:49:30Z greg.ercolano $".
//
