
/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef SkMaskFilter_DEFINED
#define SkMaskFilter_DEFINED

#include "SkFlattenable.h"
#include "SkMask.h"
#include "SkPaint.h"

class SkBlitter;
class SkBounder;
class SkMatrix;
class SkPath;
class SkRasterClip;

/** \class SkMaskFilter

    SkMaskFilter is the base class for object that perform transformations on
    an alpha-channel mask before drawing it. A subclass of SkMaskFilter may be
    installed into a SkPaint. Once there, each time a primitive is drawn, it
    is first scan converted into a SkMask::kA8_Format mask, and handed to the
    filter, calling its filterMask() method. If this returns true, then the
    new mask is used to render into the device.

    Blur and emboss are implemented as subclasses of SkMaskFilter.
*/
class SK_API SkMaskFilter : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkMaskFilter)

    SkMaskFilter() {}

    /** Returns the format of the resulting mask that this subclass will return
        when its filterMask() method is called.
    */
    virtual SkMask::Format getFormat() const = 0;

    /** Create a new mask by filter the src mask.
        If src.fImage == null, then do not allocate or create the dst image
        but do fill out the other fields in dstMask.
        If you do allocate a dst image, use SkMask::AllocImage()
        If this returns false, dst mask is ignored.
        @param  dst the result of the filter. If src.fImage == null, dst should not allocate its image
        @param src the original image to be filtered.
        @param matrix the CTM
        @param margin   if not null, return the buffer dx/dy need when calculating the effect. Used when
                        drawing a clipped object to know how much larger to allocate the src before
                        applying the filter. If returning false, ignore this parameter.
        @return true if the dst mask was correctly created.
    */
    virtual bool filterMask(SkMask* dst, const SkMask& src, const SkMatrix&,
                            SkIPoint* margin) const;

    enum BlurType {
        kNone_BlurType,    //!< this maskfilter is not a blur
        kNormal_BlurType,  //!< fuzzy inside and outside
        kSolid_BlurType,   //!< solid inside, fuzzy outside
        kOuter_BlurType,   //!< nothing inside, fuzzy outside
        kInner_BlurType    //!< fuzzy inside, nothing outside
    };

    struct BlurInfo {
        SkScalar fRadius;
        bool     fIgnoreTransform;
        bool     fHighQuality;
    };

    /**
     *  Optional method for maskfilters that can be described as a blur. If so,
     *  they return the corresponding BlurType and set the fields in BlurInfo
     *  (if not null). If they cannot be described as a blur, they return
     *  kNone_BlurType and ignore the info parameter.
     */
    virtual BlurType asABlur(BlurInfo*) const;

    /**
     * The fast bounds function is used to enable the paint to be culled early
     * in the drawing pipeline. This function accepts the current bounds of the
     * paint as its src param and the filter adjust those bounds using its
     * current mask and returns the result using the dest param. Callers are
     * allowed to provide the same struct for both src and dest so each
     * implementation must accomodate that behavior.
     *
     *  The default impl calls filterMask with the src mask having no image,
     *  but subclasses may override this if they can compute the rect faster.
     */
    virtual void computeFastBounds(const SkRect& src, SkRect* dest) const;

protected:
    // empty for now, but lets get our subclass to remember to init us for the future
    SkMaskFilter(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {}

    enum FilterReturn {
        kFalse_FilterReturn,
        kTrue_FilterReturn,
        kUnimplemented_FilterReturn
    };

    struct NinePatch {
        SkMask      fMask;      // fBounds must have [0,0] in its top-left
        SkIRect     fOuterRect; // width/height must be >= fMask.fBounds'
        SkIPoint    fCenter;    // identifies center row/col for stretching
    };

    /**
     *  Override if your subclass can filter a rect, and return the answer as
     *  a ninepatch mask to be stretched over the returned outerRect. On success
     *  return kTrue_FilterReturn. On failure (e.g. out of memory) return
     *  kFalse_FilterReturn. If the normal filterMask() entry-point should be
     *  called (the default) return kUnimplemented_FilterReturn.
     *
     *  By convention, the caller will take the center rol/col from the returned
     *  mask as the slice it can replicate horizontally and vertically as we
     *  stretch the mask to fit inside outerRect. It is an error for outerRect
     *  to be smaller than the mask's bounds. This would imply that the width
     *  and height of the mask should be odd. This is not required, just that
     *  the caller will call mask.fBounds.centerX() and centerY() to find the
     *  strips that will be replicated.
     */
    virtual FilterReturn filterRectsToNine(const SkRect[], int count,
                                           const SkMatrix&,
                                           const SkIRect& clipBounds,
                                           NinePatch*) const;

private:
    friend class SkDraw;

    /** Helper method that, given a path in device space, will rasterize it into a kA8_Format mask
     and then call filterMask(). If this returns true, the specified blitter will be called
     to render that mask. Returns false if filterMask() returned false.
     This method is not exported to java.
     */
    bool filterPath(const SkPath& devPath, const SkMatrix& devMatrix,
                    const SkRasterClip&, SkBounder*, SkBlitter* blitter,
                    SkPaint::Style style) const;

    typedef SkFlattenable INHERITED;
};

#endif
