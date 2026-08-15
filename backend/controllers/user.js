const User = require("../models/User");

exports.getProfile = async (req, res) => {
  try {
    const user = await User.getUser(req.user.id);

    if (!user) {
      return res.status(404).json({
        success: false,
        message: "User not found",
      });
    }

    res.status(200).json({
      success: true,
      user,
    });
  } catch (error) {
    console.error(error);

    res.status(500).json({
      success: false,
      message: "Failed to get profile",
    });
  }
};

exports.getUser = async (req, res) => {
  try {
    const user = await User.getUser(req.params.id);

    if (!user) {
      return res.status(404).json({
        success: false,
        message: "User not found",
      });
    }

    res.status(200).json({
      success: true,
      user,
    });
  } catch (error) {
    console.error(error);

    res.status(500).json({
      success: false,
      message: "Failed to get user",
    });
  }
};

exports.getAllUsers = async (req, res) => {
  try {
    const users = await User.getAllUsers();

    res.status(200).json({
      success: true,
      users,
    });
  } catch (error) {
    console.error(error);

    res.status(500).json({
      success: false,
      message: "Failed to get users",
    });
  }
};

exports.updateProfile = async (req, res) => {
  try {
    const { username, avatar } = req.body;

    if (
      username !== undefined &&
      (!username || username.trim().length < 3)
    ) {
      return res.status(400).json({
        success: false,
        message: "Username must be at least 3 characters",
      });
    }

    const user = await User.updateUser(req.user.id, {
      username:
        username !== undefined
          ? username.trim()
          : null,
      avatar:
        avatar !== undefined
          ? avatar
          : null,
    });

    if (!user) {
      return res.status(404).json({
        success: false,
        message: "User not found",
      });
    }

    res.status(200).json({
      success: true,
      message: "Profile updated successfully",
      user,
    });
  } catch (error) {
    console.error(error);

    if (error.code === "23505") {
      return res.status(409).json({
        success: false,
        message: "Username already exists",
      });
    }

    res.status(500).json({
      success: false,
      message: "Failed to update profile",
    });
  }
};

exports.updateOnlineStatus = async (req, res) => {
  try {
    const { isOnline } = req.body;

    if (typeof isOnline !== "boolean") {
      return res.status(400).json({
        success: false,
        message: "isOnline must be a boolean",
      });
    }

    const user = await User.updateOnlineStatus(
      req.user.id,
      isOnline
    );

    if (!user) {
      return res.status(404).json({
        success: false,
        message: "User not found",
      });
    }

    res.status(200).json({
      success: true,
      message: "Online status updated",
      user,
    });
  } catch (error) {
    console.error(error);

    res.status(500).json({
      success: false,
      message: "Failed to update online status",
    });
  }
};