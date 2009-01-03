#include "globals.h"
#include "ImageFieldState.h"


ImageFieldState::ImageFieldState(ImageField *field) : FieldState(field)
{
}

ImageFieldState::~ImageFieldState()
{
}

ImageField * ImageFieldState::getField() const
{
	return (ImageField *) FieldState::getField();
}

Size ImageFieldState::getPreferedSize() const
{
	HBITMAP hBmp = getImage();
	BITMAP bmp;
	if (hBmp == NULL || GetObject(hBmp, sizeof(bmp), &bmp) == 0)
		return Size(0, 0);

	return Size(bmp.bmWidth, bmp.bmHeight);
}

HBITMAP ImageFieldState::getImage() const
{
	return getField()->getImage();
}

bool ImageFieldState::isEmpty() const
{
	return getImage() != NULL;
}