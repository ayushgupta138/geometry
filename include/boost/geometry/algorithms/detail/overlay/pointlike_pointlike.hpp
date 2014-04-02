// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle


#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_POINTLIKE_POINTLIKE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_POINTLIKE_POINTLIKE_HPP

#include <algorithm>

#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/algorithms/detail/relate/less.hpp>
#include <boost/geometry/algorithms/detail/disjoint/point_point.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


// struct for copying points of the pointlike geometries to output
template
<
    typename PointOut,
    typename GeometryIn,
    typename TagIn = typename tag<GeometryIn>::type
>
struct copy_points
    : not_implemented<PointOut, GeometryIn>
{};

template <typename PointOut, typename PointIn>
struct copy_points<PointOut, PointIn, point_tag>
{
    template <typename OutputIterator>
    static inline void apply(PointIn const& point_in,
                             OutputIterator& oit)
    {
        PointOut point_out;
        geometry::convert(point_in, point_out);
        *oit++ = point_out;
    }
};


template <typename PointOut, typename MultiPointIn>
struct copy_points<PointOut, MultiPointIn, multi_point_tag>
{
    template <typename OutputIterator>
    static inline void apply(MultiPointIn const& multi_point_in,
                             OutputIterator& oit)
    {
        BOOST_AUTO_TPL(it, boost::begin(multi_point_in));
        for (; it != boost::end(multi_point_in); ++it)
        {
            PointOut point_out;
            geometry::convert(*it, point_out);
            *oit++ = point_out;
        }
    }
};



// action struct for difference/intersection
template <typename PointOut, overlay_type OverlayType>
struct action_selector_pl_pl
{};

template <typename PointOut>
struct action_selector_pl_pl<PointOut, overlay_intersection>
{
    template
    <
        typename Point,
        typename OutputIterator
    >
    static inline void apply(Point const& point,
                             bool is_common,
                             OutputIterator& oit)
    {
        if ( is_common )
        {
            copy_points<PointOut, Point>::apply(point, oit);
        }
    }
};



template <typename PointOut>
struct action_selector_pl_pl<PointOut, overlay_difference>
{
    template
    <
        typename Point,
        typename OutputIterator
    >
    static inline void apply(Point const& point,
                             bool is_common,
                             OutputIterator& oit)
    {
        if ( !is_common )
        {
            copy_points<PointOut, Point>::apply(point, oit);
        }
    }
};


//===========================================================================
//===========================================================================
//===========================================================================

// difference/intersection of point-point
template
<
    typename Point1,
    typename Point2,
    typename PointOut,
    overlay_type OverlayType
>
struct point_point_point
{
    template <typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Point1 const& point1,
                                       Point2 const& point2,
                                       OutputIterator oit,
                                       Strategy const&)
    {
        action_selector_pl_pl
            <
                PointOut, OverlayType
            >::apply(point1,
                     detail::equals::equals_point_point(point1, point2),
                     oit);

        return oit;
    }
};



// difference of multipoint-point
//
// the apply method in the following struct is called only for
// difference; for intersection the reversal will
// always call the point-multipoint version
template
<
    typename MultiPoint,
    typename Point,
    typename PointOut,
    overlay_type OverlayType
>
struct multipoint_point_point
{
    template <typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(MultiPoint const& multipoint,
                                       Point const& point,
                                       OutputIterator oit,
                                       Strategy const&)
    {
        BOOST_ASSERT( OverlayType == overlay_difference );

        BOOST_AUTO_TPL(it, boost::begin(multipoint));
        for (; it != boost::end(multipoint); ++it)
        {
            action_selector_pl_pl
                <
                    PointOut, OverlayType
                >::apply(*it,
                         detail::equals::equals_point_point(*it, point),
                         oit);
        }

        return oit;
    }
};


// difference/intersection of point-multipoint
template
<
    typename Point,
    typename MultiPoint,
    typename PointOut,
    overlay_type OverlayType
>
struct point_multipoint_point
{
    template <typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Point const& point,
                                       MultiPoint const& multipoint,
                                       OutputIterator oit,
                                       Strategy const&)
    {
        typedef action_selector_pl_pl<PointOut, OverlayType> action;

        BOOST_AUTO_TPL(it, boost::begin(multipoint));
        for (; it != boost::end(multipoint); ++it)
        {
            if ( detail::equals::equals_point_point(*it, point) )
            {
                action::apply(point, true, oit);
                return oit;
            }
        }

        action::apply(point, false, oit);
        return oit;
    }
};



// difference/intersection of multipoint-multipoint
template
<
    typename MultiPoint1,
    typename MultiPoint2,
    typename PointOut,
    overlay_type OverlayType
>
struct multipoint_multipoint_point
{
    template <typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(MultiPoint1 const& multipoint1,
                                       MultiPoint2 const& multipoint2,
                                       OutputIterator oit,
                                       Strategy const& strategy)
    {
        if ( OverlayType != overlay_difference
             && boost::size(multipoint1) > boost::size(multipoint2) )
        {
            return multipoint_multipoint_point
                <
                    MultiPoint2, MultiPoint1, PointOut, OverlayType
                >::apply(multipoint2, multipoint1, oit, strategy);
        }

        std::vector<typename point_type<MultiPoint2>::type>
            points2(boost::begin(multipoint2), boost::end(multipoint2));

        std::sort(points2.begin(), points2.end(), detail::relate::less());

        BOOST_AUTO_TPL(it1, boost::begin(multipoint1));
        for (; it1 != boost::end(multipoint1); ++it1)
        {
            bool found = std::binary_search(points2.begin(), points2.end(),
                                            *it1, detail::relate::less());

            action_selector_pl_pl
                <
                    PointOut, OverlayType
                >::apply(*it1, found, oit);
        }
        return oit;
    }
};



//===========================================================================
//===========================================================================
//===========================================================================


// dispatch struct for pointlike-pointlike difference/intersection
// computation
template
<
    typename PointLike1,
    typename PointLike2,
    typename PointOut,
    overlay_type OverlayType,
    typename Tag1,
    typename Tag2
>
struct pointlike_pointlike_point
    : not_implemented<PointLike1, PointLike2, PointOut>
{};


template
<
    typename Point1,
    typename Point2,
    typename PointOut,
    overlay_type OverlayType
>
struct pointlike_pointlike_point
    <
        Point1, Point2, PointOut, OverlayType,
        point_tag, point_tag
    > : point_point_point<Point1, Point2, PointOut, OverlayType>
{};


template
<
    typename Point,
    typename MultiPoint,
    typename PointOut,
    overlay_type OverlayType
>
struct pointlike_pointlike_point
    <
        Point, MultiPoint, PointOut, OverlayType,
        point_tag, multi_point_tag
    > : point_multipoint_point<Point, MultiPoint, PointOut, OverlayType>
{};


template
<
    typename MultiPoint,
    typename Point,
    typename PointOut,
    overlay_type OverlayType
>
struct pointlike_pointlike_point
    <
        MultiPoint, Point, PointOut, OverlayType,
        multi_point_tag, point_tag
    > : multipoint_point_point<MultiPoint, Point, PointOut, OverlayType>
{};


template
<
    typename MultiPoint1,
    typename MultiPoint2,
    typename PointOut,
    overlay_type OverlayType
>
struct pointlike_pointlike_point
    <
        MultiPoint1, MultiPoint2, PointOut, OverlayType,
        multi_point_tag, multi_point_tag
    > : multipoint_multipoint_point
        <
            MultiPoint1, MultiPoint2, PointOut, OverlayType
        >
{};



//===========================================================================
//===========================================================================
//===========================================================================

// generic pointlike-pointlike union implementation
template
<
    typename PointLike1,
    typename PointLike2,
    typename PointOut
>
struct union_pointlike_pointlike_point
{
    template <typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(PointLike1 const& pointlike1,
                                       PointLike2 const& pointlike2,
                                       OutputIterator oit,
                                       Strategy const& strategy)
    {
        copy_points<PointOut, PointLike1>::apply(pointlike1, oit);

        return pointlike_pointlike_point
            <
                PointLike2, PointLike1, PointOut, overlay_difference,
                typename tag<PointLike2>::type,
                typename tag<PointLike1>::type
            >::apply(pointlike2, pointlike1, oit, strategy);
    }

};


// generic pointlike-pointlike difference/intersection implementation
// this is just a wrapper class
template
<
    typename PointLike1,
    typename PointLike2,
    typename PointOut,
    overlay_type OverlayType
>
struct difference_intersection_pointlike_pointlike_point
    : pointlike_pointlike_point
        <
            PointLike1, PointLike2, PointOut, OverlayType,
            typename tag<PointLike1>::type,
            typename tag<PointLike2>::type
        >
{};



}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry



#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_POINTLIKE_POINTLIKE_HPP
