/**
 * This file is part of the "clip" project
 *   Copyright (c) 2018 Paul Asmuth
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "errorbars.h"

#include "data.h"
#include "context.h"
#include "layout.h"
#include "graphics/path.h"
#include "graphics/brush.h"
#include "graphics/text.h"
#include "graphics/layout.h"
#include "scale.h"
#include "color_reader.h"
#include "sexpr_conv.h"
#include "sexpr_util.h"

#include <numeric>

using namespace std::placeholders;
using std::bind;

namespace clip::plotgen {

static const double kDefaultStrokeWidthPT = 1;
static const double kDefaultBarWidthPT = 6;

struct ErrorbarsElement {
  std::vector<Measure> x;
  std::vector<Measure> x_low;
  std::vector<Measure> x_high;
  std::vector<Measure> y;
  std::vector<Measure> y_low;
  std::vector<Measure> y_high;
  ScaleConfig scale_x;
  ScaleConfig scale_y;
  Measure bar_width;
  std::vector<Color> colors;
  Color stroke_color;
  Measure stroke_width;
};

ReturnCode errorbars_draw_bar(
    Context* ctx,
    const ErrorbarsElement& config,
    const Point& from,
    const Point& to,
    const Color& color) {
  auto direction = normalize(sub(to, from));
  auto ortho = vec2{direction.y, direction.x * -1};

  StrokeStyle line_style;
  line_style.color = color;
  line_style.line_width = config.stroke_width;

  draw_line(ctx, from, to, line_style);

  draw_line(
      ctx,
      add(from, mul(ortho, config.bar_width * -0.5)),
      add(from, mul(ortho, config.bar_width * 0.5)),
      line_style);

  draw_line(
      ctx,
      add(to, mul(ortho, config.bar_width * -0.5)),
      add(to, mul(ortho, config.bar_width * 0.5)),
      line_style);

  return OK;
}

ReturnCode errorbars_draw(
    Context* ctx,
    PlotConfig* plot,
    ErrorbarsElement* config) {
  const auto& clip = plot_get_clip(plot, layer_get(ctx));

  /* convert units */
  convert_units(
      {
        std::bind(&convert_unit_typographic, layer_get_dpi(ctx), layer_get_font_size(ctx), _1),
        std::bind(&convert_unit_user, scale_translate_fn(config->scale_x), _1),
        std::bind(&convert_unit_relative, clip.w, _1)
      },
      &*config->x.begin(),
      &*config->x.end());

  convert_units(
      {
        std::bind(&convert_unit_typographic, layer_get_dpi(ctx), layer_get_font_size(ctx), _1),
        std::bind(&convert_unit_user, scale_translate_fn(config->scale_x), _1),
        std::bind(&convert_unit_relative, clip.w, _1)
      },
      &*config->x_low.begin(),
      &*config->x_low.end());

  convert_units(
      {
        std::bind(&convert_unit_typographic, layer_get_dpi(ctx), layer_get_font_size(ctx), _1),
        std::bind(&convert_unit_user, scale_translate_fn(config->scale_x), _1),
        std::bind(&convert_unit_relative, clip.w, _1)
      },
      &*config->x_high.begin(),
      &*config->x_high.end());

  convert_units(
      {
        std::bind(&convert_unit_typographic, layer_get_dpi(ctx), layer_get_font_size(ctx), _1),
        std::bind(&convert_unit_user, scale_translate_fn(config->scale_y), _1),
        std::bind(&convert_unit_relative, clip.h, _1)
      },
      &*config->y.begin(),
      &*config->y.end());

  convert_units(
      {
        std::bind(&convert_unit_typographic, layer_get_dpi(ctx), layer_get_font_size(ctx), _1),
        std::bind(&convert_unit_user, scale_translate_fn(config->scale_y), _1),
        std::bind(&convert_unit_relative, clip.h, _1)
      },
      &*config->y_low.begin(),
      &*config->y_low.end());

  convert_units(
      {
        std::bind(&convert_unit_typographic, layer_get_dpi(ctx), layer_get_font_size(ctx), _1),
        std::bind(&convert_unit_user, scale_translate_fn(config->scale_y), _1),
        std::bind(&convert_unit_relative, clip.h, _1)
      },
      &*config->y_high.begin(),
      &*config->y_high.end());

  convert_unit_typographic(
      layer_get_dpi(ctx),
      layer_get_font_size(ctx),
      &config->stroke_width);

  convert_unit_typographic(
      layer_get_dpi(ctx),
      layer_get_font_size(ctx),
      &config->bar_width);

  auto x_len = std::min({
    config->x.size(),
    config->y_low.size(),
    config->y_high.size(),
  });

  auto y_len = std::min({
    config->y.size(),
    config->x_low.size(),
    config->x_high.size(),
  });

  for (size_t i = 0; i < x_len; ++i) {
    auto from = vec2(
        clip.x + config->x[i],
        clip.y + config->y_low[i]);

    auto to = vec2(
        clip.x + config->x[i],
        clip.y + config->y_high[i]);

    const auto& color = config->colors.empty()
        ? config->stroke_color
        : config->colors[i % config->colors.size()];

    if (auto rc = errorbars_draw_bar(ctx, *config, from, to, color); !rc) {
      return rc;
    }
  }

  for (size_t i = 0; i < y_len; ++i) {
    auto from = vec2(
        clip.x + config->x_low[i],
        clip.y + config->y[i]);

    auto to = vec2(
        clip.x + config->x_high[i],
        clip.x + config->y[i]);

    const auto& color = config->colors.empty()
        ? config->stroke_color
        : config->colors[i % config->colors.size()];

    if (auto rc = errorbars_draw_bar(ctx, *config, from, to, color); !rc) {
      return rc;
    }
  }

  return OK;
}

ReturnCode errorbars_configure(
    Context* ctx,
    PlotConfig* plot,
    ErrorbarsElement* c,
    const Expr* expr) {
  /* set defaults from environment */
  c->stroke_width = from_pt(kDefaultStrokeWidthPT);
  c->bar_width = from_pt(kDefaultBarWidthPT);
  c->stroke_color = layer_get(ctx)->foreground_color;
  c->scale_x = plot->scale_x;
  c->scale_y = plot->scale_y;

  /* parse properties */
  std::vector<std::string> data_x;
  std::vector<std::string> data_x_low;
  std::vector<std::string> data_x_high;
  std::vector<std::string> data_y;
  std::vector<std::string> data_y_low;
  std::vector<std::string> data_y_high;
  std::vector<std::string> data_colors;
  ColorMap color_map;

  auto config_rc = expr_walk_map_wrapped(expr, {
    {"data-x", std::bind(&data_load_strings, _1, &data_x)},
    {"data-x-low", std::bind(&data_load_strings, _1, &data_x_low)},
    {"data-x-high", std::bind(&data_load_strings, _1, &data_x_high)},
    {"data-y", std::bind(&data_load_strings, _1, &data_y)},
    {"data-y-low", std::bind(&data_load_strings, _1, &data_y_low)},
    {"data-y-high", std::bind(&data_load_strings, _1, &data_y_high)},
    {"limit-x", std::bind(&expr_to_float64_opt_pair, _1, &c->scale_x.min, &c->scale_x.max)},
    {"limit-x-min", std::bind(&expr_to_float64_opt, _1, &c->scale_x.min)},
    {"limit-x-max", std::bind(&expr_to_float64_opt, _1, &c->scale_x.max)},
    {"limit-y", std::bind(&expr_to_float64_opt_pair, _1, &c->scale_y.min, &c->scale_y.max)},
    {"limit-y-min", std::bind(&expr_to_float64_opt, _1, &c->scale_y.min)},
    {"limit-y-max", std::bind(&expr_to_float64_opt, _1, &c->scale_y.max)},
    {"scale-x", std::bind(&scale_configure_kind, _1, &c->scale_x)},
    {"scale-y", std::bind(&scale_configure_kind, _1, &c->scale_y)},
    {"scale-x-padding", std::bind(&expr_to_float64, _1, &c->scale_x.padding)},
    {"scale-y-padding", std::bind(&expr_to_float64, _1, &c->scale_y.padding)},
    {"width", std::bind(&measure_read, _1, &c->bar_width)},
    {"color", std::bind(&color_read, ctx, _1, &c->stroke_color)},
    {"colors", std::bind(&data_load_strings, _1, &data_colors)},
    {"color-map", std::bind(&color_map_read, ctx, _1, &color_map)},
    {"stroke-color", std::bind(&color_read, ctx, _1, &c->stroke_color)},
    {"stroke-width", std::bind(&measure_read, _1, &c->stroke_width)}
  });

  if (!config_rc) {
    return config_rc;
  }

  /* figure out in which direction the user wants us to plot the error bars */
  Option<Direction> direction;
  if (!data_x.empty() &&
      !data_y_low.empty() &&
      !data_y_high.empty() &&
      data_y.empty()) {
    direction = Direction::VERTICAL;

    if (data_x.size() != data_y_low.size() ||
        data_x.size() != data_y_high.size()) {
      return error(
          ERROR,
          "the length of the 'data-x', 'data-y-low' and 'data-y-high' datasets "
          "must be equal");
    }
  }

  if (!data_y.empty() &&
      !data_x_low.empty() &&
      !data_x_high.empty() &&
      data_x.empty()) {
    direction = Direction::HORIZONTAL;

    if (data_y.size() != data_x_low.size() ||
        data_y.size() != data_x_high.size()) {
      return error(
          ERROR,
          "the length of the 'data-x', 'data-y-low' and 'data-y-high' datasets "
          "must be equal");
    }
  }

  if (!direction) {
    return error(
        ERROR,
        "the errorbars expectes exactly three data properties:\n"
        "  - vertical bars: data-x, data-y-low, data-y-high\n"
        "  - horizontal bars: data-y, data-x-low, data-x-high");
  }

  /* scale autoconfiguration */
  if (auto rc = data_to_measures(data_x, c->scale_x, &c->x); !rc){
    return rc;
  }

  if (auto rc = data_to_measures(data_x_low, c->scale_x, &c->x_low); !rc){
    return rc;
  }

  if (auto rc = data_to_measures(data_x_high, c->scale_x, &c->x_high); !rc){
    return rc;
  }

  if (auto rc = data_to_measures(data_y, c->scale_y, &c->y); !rc){
    return rc;
  }

  if (auto rc = data_to_measures(data_y_low, c->scale_y, &c->y_low); !rc){
    return rc;
  }

  if (auto rc = data_to_measures(data_y_high, c->scale_y, &c->y_high); !rc){
    return rc;
  }

  for (const auto& v : c->x) {
    if (v.unit == Unit::USER) {
      scale_fit(v.value, &c->scale_x);
    }
  }

  for (const auto& v : c->x_low) {
    if (v.unit == Unit::USER) {
      scale_fit(v.value, &c->scale_x);
    }
  }

  for (const auto& v : c->x_high) {
    if (v.unit == Unit::USER) {
      scale_fit(v.value, &c->scale_x);
    }
  }

  for (const auto& v : c->y) {
    if (v.unit == Unit::USER) {
      scale_fit(v.value, &c->scale_y);
    }
  }

  for (const auto& v : c->y_low) {
    if (v.unit == Unit::USER) {
      scale_fit(v.value, &c->scale_y);
    }
  }

  for (const auto& v : c->y_high) {
    if (v.unit == Unit::USER) {
      scale_fit(v.value, &c->scale_y);
    }
  }

  /* convert color data */
  for (const auto& value : data_colors) {
    Color color;
    if (color_map) {
      if (auto rc = color_map(value, &color); !rc) {
        return rc;
      }
    } else {
      if (auto rc = color.parse(value); !rc) {
        return errorf(
            ERROR,
            "invalid data; can't parse '{}' as a color hex code; maybe you "
            "forgot to set the 'color-map' option?",
            value);
      }
    }

    c->colors.push_back(color);
  }


  return OK;
}

ReturnCode errorbars_draw(
    Context* ctx,
    PlotConfig* plot,
    const Expr* expr) {
  ErrorbarsElement conf;

  if (auto rc = errorbars_configure(ctx, plot, &conf, expr); !rc) {
    return rc;
  }

  return errorbars_draw(ctx, plot, &conf);
}

ReturnCode errorbars_autorange(
    Context* ctx,
    PlotConfig* plot,
    const Expr* expr) {
  ErrorbarsElement conf;
  return errorbars_configure(ctx, plot, &conf, expr);
}

} // namespace clip::plotgen

