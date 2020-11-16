#ifndef DUCKTYPES_H
#define DUCKTYPES_H

/**
 * @brief Type of ducks
 *
 */
enum DuckType {
  /// A Duck of unknown type
  UNKNOWN = 0x00,
  /// A PapaDuck
  PAPA = 0x01,
  /// A MamaDuck
  MAMA = 0x02,
  /// A DuckLink
  LINK = 0x03,
  /// A Detector Duck
  DETECTOR = 0x04,
  MAX_TYPE
};

#endif