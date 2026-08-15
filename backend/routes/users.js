const express = require("express");
const router = express.Router();

const userController = require("../controllers/user");
const verifyToken = require("../middlewares/verifyToken");

router.get("/profile", verifyToken, userController.getProfile);

router.get("/users", verifyToken, userController.getAllUsers);

router.get("/users/:id", verifyToken, userController.getUser);

router.patch("/profile", verifyToken, userController.updateProfile);

router.patch(
  "/online-status",
  verifyToken,
  userController.updateOnlineStatus
);

module.exports = router;