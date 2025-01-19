import os
import time
import subprocess
import shutil

PHOTO_DIR = "/home/wigo/Pictures/testing"


# Configuration
PHOTO_DIR = "/home/wigo/Pictures/testing"
UPLOADED_DIR = "/home/wigo/Pictures/uploaded"  # Replace with the folder path where uploaded pictures will be moved
UPLOAD_URL = "https://projects.benax.rw/f/o/r/e/a/c/h/p/r/o/j/e/c/t/s/4e8d42b606f70fa9d39741a93ed0356c/iot_testing_202501/upload.php"
SUPPORTED_EXTENSIONS = {"jpg", "jpeg", "png"}  # Define supported image extensions

def is_supported_image(file_name):
    """Check if the file has a supported image extension."""
    extension = os.path.splitext(file_name)[1][1:].lower()  # Get extension without the dot
    return extension in SUPPORTED_EXTENSIONS

def upload_photo(photo_path):
    """Upload a photo using the curl command."""
    try:
        # Use the curl command to upload the image
        result = subprocess.run(
            ["curl", "-X", "POST", "-F", f"imageFile=@{photo_path}", UPLOAD_URL],
            capture_output=True, text=True
        )
        # Check if the upload was successful
        if result.returncode == 0:
            print(f"Uploaded {photo_path} successfully!")
            return True
        else:
            print(f"Failed to upload {photo_path}. Error: {result.stderr}")
            return False
    except Exception as e:
        print(f"Error uploading {photo_path}: {e}")
        return False

def move_to_uploaded(photo_path):
    """Move an uploaded photo to the uploaded folder."""
    try:
        if not os.path.exists(UPLOADED_DIR):
            os.makedirs(UPLOADED_DIR)  # Create the folder if it doesn't exist
        shutil.move(photo_path, os.path.join(UPLOADED_DIR, os.path.basename(photo_path)))
        print(f"Moved {photo_path} to {UPLOADED_DIR}")
    except Exception as e:
        print(f"Error moving {photo_path}: {e}")

def main():
    """Main function to monitor the folder and upload images."""
    print("Starting folder monitoring...")
    while True:
        for photo in os.listdir(PHOTO_DIR):
            photo_path = os.path.join(PHOTO_DIR, photo)
            
            # Check if it's a file and a supported image
            if os.path.isfile(photo_path) and is_supported_image(photo):
                print(f"Uploading {photo_path}...")
                if upload_photo(photo_path):
                    move_to_uploaded(photo_path)
            else:
                print(f"Skipping {photo_path} (not an image or not supported).")
        
        print("Waiting 30 seconds before checking again...")
        time.sleep(30)

if __name__ == "__main__":
    main()
