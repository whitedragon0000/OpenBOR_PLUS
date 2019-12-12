package org.openbor.engine.utils;

public class FrameDimensions {

  private int x;

  private int y;

  private int width;

  private int height;

  private int top;

  private int left;

  private int bottom;

  private int right;

  public FrameDimensions() {

  }

  public FrameDimensions(int x, int y, int width, int height,
                         int top, int left, int bottom, int right) {
    this.x = x;
    this.y = y;
    this.width = width;
    this.height = height;
    this.top = top;
    this.left = left;
    this.bottom = bottom;
    this.right = right;
  }

  public int getX() {
    return this.x;
  }

  public void setX(int x) {
    this.x = x;
  }

  public int getY() {
    return this.y;
  }

  public void setY(int y) {
    this.y = y;
  }

  public int getWidth() {
    return this.width;
  }

  public void setWidth(int width) {
    this.width = width;
  }

  public int getHeight() {
    return this.height;
  }

  public void setHeight(int height) {
    this.height = height;
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
