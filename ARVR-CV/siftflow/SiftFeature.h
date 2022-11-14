#ifndef __ICS_HAR_VISION_SIFT_FEATURE__
#define __ICS_HAR_VISION_SIFT_FEATURE__

namespace ics{
  namespace har{
    namespace vision{
      class SiftFeature{
      public:
        virtual double getX() const = 0;
        virtual double getY() const = 0;
        virtual double getDimension() const = 0;
        virtual double distance(const SiftFeature* feature) const = 0;
        virtual double getScale() const = 0;
        virtual double getOrientation() const = 0;
        virtual ~SiftFeature(){}
      };
    }
  }
}

#endif
