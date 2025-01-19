import os
import time
import requests

# Directory containing photos to upload
PHOTO_DIR = "/home/wigo/Pictures/testing"

# URL for uploading photos
UPLOAD_URL = "https://projects.benax.rw/f/o/r/e/a/c/h/p/r/o/j/e/c/t/s/4e8d42b606f70fa9d39741a93ed0356c/iot_testing_202501/upload.php"

# Supported photo extensions
SUPPORTED_EXTENSIONS = {"jpg"}


def is_supported_image(file_name):
    """Check if the file has a supported image extension."""
    extension = os.path.splitext(file_name)[1][
        1:
    ].lower()  # Get extension without the dot and convert to lowercase
    return extension in SUPPORTED_EXTENSIONS


def upload_photo(photo_path):
    """Upload a photo to the specified URL."""
    try:
        with open(photo_path, "rb") as file:
            response = requests.post(
                UPLOAD_URL, files={"imageFile": file}, verify=False
            )
        if response.status_code == 200:
            print(f"Uploaded {photo_path} successfully!")
        else:
            print(f"Failed to upload {photo_path}. Status code: {response.status_code}")
    except Exception as e:
        print(f"Error uploading {photo_path}: {e}")


def main():
    """Main function to upload photos every 30 seconds."""
    while True:
        for photo in os.listdir(PHOTO_DIR):
            photo_path = os.path.join(PHOTO_DIR, photo)

            # Check if it's a file and a supported image
            if os.path.isfile(photo_path) and is_supported_image(photo):
                print(f"Uploading {photo_path}...")
                upload_photo(photo_path)
            else:
                print(f"Skipping {photo_path} (not an image or not supported).")

        print("Waiting 30 seconds before the next upload...")
        time.sleep(30)


if __name__ == "__main__":
    main()
