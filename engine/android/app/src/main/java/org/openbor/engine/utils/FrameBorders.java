package org.openbor.engine.utils;

public class FrameBorders {

  private int top;

  private int left;

  private int bottom;

  private int right;

  public FrameBorders() {

  }

  public FrameBorders(int top, int left, int bottom, int right) {
    this.top = top;
    this.left = left;
    this.bottom = bottom;
    this.right = right;
  }

  public int getTop() {
    return this.top;
  }

  public void setTop(int top) {
    this.top = top;
  }

  public int getLeft() {
    return this.left;
  }

  public void setLeft(int left) {
    this.left = left;
  }

  public int getBottom() {
    return this.bottom;
  }

  public void setBottom(int bottom) {
    this.bottom = bottom;
  }

  public int getRight() {
    return this.right;
  }

  public void setRight(int right) {
    this.right = right;
  }

}
